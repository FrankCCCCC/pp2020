#include <iostream>
#include <algorithm>
#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <driver_functions.h>

// #include "floyd_warshall.hpp"

#define BLOCK_DIM 16
#define SIZEOFINT sizeof(int)
const int INF = ((1 << 30) - 1);
// const int B = 64;
int n, m;
int *Dist;
int *Dist_out;
// int *Dist_cuda;

__forceinline__
__host__ void check_cuda_error() {
  cudaError_t errCode = cudaPeekAtLastError();
  if (errCode != cudaSuccess) {
    std::cerr << "WARNING: A CUDA error occured: code=" << errCode << "," <<
                cudaGetErrorString(errCode) << "\n";
  }
}

__forceinline__
__device__ void calc(int* graph, int n, int k, int i, int j) {
  if ((i >= n) || (j >= n) || (k >= n)) return;
  const unsigned int kj = k*n + j;
  const unsigned int ij = i*n + j;
  const unsigned int ik = i*n + k;
  int t1 = graph[ik] + graph[kj];
  int t2 = graph[ij];
  graph[ij] = (t1 < t2) ? t1 : t2;
}


__global__ void floyd_warshall_kernel(int n, int k, int* graph) {
  const unsigned int i = blockIdx.y * blockDim.y + threadIdx.y;
  const unsigned int j = blockIdx.x * blockDim.x + threadIdx.x;
  calc(graph, n, k, i, j);
}

/*****************************************************************************
                         Blocked Floyd-Warshall Kernel
  ***************************************************************************/

__forceinline__
__device__ void block_calc(int* C, int* A, int* B, int bj, int bi) {
  for (int k = 0; k < BLOCK_DIM; k++) {
    int sum = A[bi*BLOCK_DIM + k] + B[k*BLOCK_DIM + bj];
    if (C[bi*BLOCK_DIM + bj] > sum) {
      C[bi*BLOCK_DIM + bj] = sum;
    }
    __syncthreads();
  }
}

__global__ void floyd_warshall_block_kernel_phase1(int n, int k, int* graph) {
  const unsigned int bi = threadIdx.y;
  const unsigned int bj = threadIdx.x;

  __shared__ int C[BLOCK_DIM * BLOCK_DIM];

  __syncthreads();

  // Transfer to temp shared arrays
  C[bi*BLOCK_DIM + bj] = graph[k*BLOCK_DIM*n + k*BLOCK_DIM + bi*n + bj];

  __syncthreads();
  
  block_calc(C, C, C, bi, bj);

  __syncthreads();

  // Transfer back to graph
  graph[k*BLOCK_DIM*n + k*BLOCK_DIM + bi*n + bj] = C[bi*BLOCK_DIM + bj];

}


__global__ void floyd_warshall_block_kernel_phase2(int n, int k, int* graph) {
  // BlockDim is one dimensional (Straight along diagonal)
  // Blocks themselves are two dimensional
  const unsigned int i = blockIdx.x;
  const unsigned int bi = threadIdx.y;
  const unsigned int bj = threadIdx.x;

  if (i == k) return;

  __shared__ int A[BLOCK_DIM * BLOCK_DIM];
  __shared__ int B[BLOCK_DIM * BLOCK_DIM];
  __shared__ int C[BLOCK_DIM * BLOCK_DIM];

  __syncthreads();

  C[bi*BLOCK_DIM + bj] = graph[i*BLOCK_DIM*n + k*BLOCK_DIM + bi*n + bj];
  B[bi*BLOCK_DIM + bj] = graph[k*BLOCK_DIM*n + k*BLOCK_DIM + bi*n + bj];

  __syncthreads();

  block_calc(C, C, B, bi, bj);

  __syncthreads();

  graph[i*BLOCK_DIM*n + k*BLOCK_DIM + bi*n + bj] = C[bi*BLOCK_DIM + bj];

  // Phase 2 1/2

  C[bi*BLOCK_DIM + bj] = graph[k*BLOCK_DIM*n + i*BLOCK_DIM + bi*n + bj];
  A[bi*BLOCK_DIM + bj] = graph[k*BLOCK_DIM*n + k*BLOCK_DIM + bi*n + bj];

  __syncthreads();

  block_calc(C, A, C, bi, bj);

  __syncthreads();

  // Block C is the only one that could be changed
  graph[k*BLOCK_DIM*n + i*BLOCK_DIM + bi*n + bj] = C[bi*BLOCK_DIM + bj];
}


__global__ void floyd_warshall_block_kernel_phase3(int n, int k, int* graph) {
  // BlockDim is one dimensional (Straight along diagonal)
  // Blocks themselves are two dimensional
  const unsigned int j = blockIdx.x;
  const unsigned int i = blockIdx.y;
  const unsigned int bi = threadIdx.y;
  const unsigned int bj = threadIdx.x;

  if (i == k && j == k) return;
  __shared__ int A[BLOCK_DIM * BLOCK_DIM];
  __shared__ int B[BLOCK_DIM * BLOCK_DIM];
  __shared__ int C[BLOCK_DIM * BLOCK_DIM];

  __syncthreads();

  C[bi*BLOCK_DIM + bj] = graph[i*BLOCK_DIM*n + j*BLOCK_DIM + bi*n + bj];
  A[bi*BLOCK_DIM + bj] = graph[i*BLOCK_DIM*n + k*BLOCK_DIM + bi*n + bj];
  B[bi*BLOCK_DIM + bj] = graph[k*BLOCK_DIM*n + j*BLOCK_DIM + bi*n + bj];

  __syncthreads();

  block_calc(C, A, B, bi, bj);

  __syncthreads();

  graph[i*BLOCK_DIM*n + j*BLOCK_DIM + bi*n + bj] = C[bi*BLOCK_DIM + bj];
}

/************************************************************************
                    Floyd-Warshall's Algorithm CUDA
************************************************************************/


__host__ void floyd_warshall_blocked_cuda(int* input, int* output, int n) {

  int deviceCount;
  cudaGetDeviceCount(&deviceCount);

  for (int i = 0; i < deviceCount; i++) {
    cudaDeviceProp deviceProps;
    cudaGetDeviceProperties(&deviceProps, i);

    std::cout << "Device " << i << ": " << deviceProps.name << "\n"
	      << "\tSMs: " << deviceProps.multiProcessorCount << "\n"
	      << "\tGlobal mem: " << static_cast<float>(deviceProps.totalGlobalMem) / (1024 * 1024 * 1024) << "GB \n"
	      << "\tCUDA Cap: " << deviceProps.major << "." << deviceProps.minor << "\n";
  }

  int* device_graph;
  const size_t size = sizeof(int) * n * n;
  cudaMalloc(&device_graph, size);
  cudaMemcpy(device_graph, input, size, cudaMemcpyHostToDevice);

  const int blocks = (n + BLOCK_DIM - 1) / BLOCK_DIM;
  dim3 block_dim(BLOCK_DIM, BLOCK_DIM, 1);
  dim3 phase4_grid(blocks, blocks, 1);

  std::cout << "Launching Kernels Blocks: " << blocks << " Size " << n << "\n";
  for (int k = 0; k < blocks; k++) {
    floyd_warshall_block_kernel_phase1<<<1, block_dim>>>(n, k, device_graph);

    floyd_warshall_block_kernel_phase2<<<blocks, block_dim>>>(n, k, device_graph);

    floyd_warshall_block_kernel_phase3<<<phase4_grid, block_dim>>>(n, k, device_graph);
  }
  
  cudaMemcpy(output, device_graph, size, cudaMemcpyDeviceToHost);
  check_cuda_error();

  cudaFree(device_graph);
}

void malloc_Dist(int vertex_num){
  Dist = (int*)malloc(SIZEOFINT * vertex_num * vertex_num);
  Dist_out = (int*)malloc(SIZEOFINT * vertex_num * vertex_num);
}
int getDist(int i, int j, int vertex_num){return Dist[i * vertex_num + j];}
int *getDistAddr(int i, int j, int vertex_num){return &(Dist[i * vertex_num + j]);}
void setDist(int i, int j, int val, int vertex_num){Dist[i * vertex_num + j] = val;}

void input(char* infile) {
    FILE* file = fopen(infile, "rb");
    fread(&n, sizeof(int), 1, file);
    fread(&m, sizeof(int), 1, file);
    malloc_Dist(n);
    // malloc_DistCuda(n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                setDist(i, j, 0, n);
                // Dist[i][j] = 0;
            } else {
                setDist(i, j, INF, n);
                // Dist[i][j] = INF;
            }
        }
    }

    int pair[3];
    for (int i = 0; i < m; i++) {
        fread(pair, sizeof(int), 3, file);
        setDist(pair[0], pair[1], pair[2], n);
        // Dist[pair[0]][pair[1]] = pair[2];
    }
    // cudaMemcpy(Dist_cuda, Dist, (n * n * SIZEOFINT), cudaMemcpyHostToDevice);
    fclose(file);
}

void output(char* outFileName) {
    FILE* outfile = fopen(outFileName, "w");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            // if (Dist[i][j] >= INF) Dist[i][j] = INF;
            if (getDist(i, j, n) >= INF) setDist(i, j, INF, n);
        }
        // fwrite(Dist[i], sizeof(int), n, outfile);
        // fwrite(getDistAddr(i, 0, n), sizeof(int), n, outfile);
    }
    fwrite(getDistAddr(0, 0, n), sizeof(int), n * n, outfile);
    fclose(outfile);
}

int main(int argc, char* argv[]) {
    input(argv[1]);
    // show_mat(getDistAddr(0, 0, n), n);
    // setup_DistCuda(n);
    printf("Vertice: %d, Edge: %d\n", n, m);
    floyd_warshall_blocked_cuda(Dist, Dist_out, n);
    // block_FW_cuda(B);
    // back_DistCuda(n);
    // show_mat(getDistAddr(0, 0, n), n);
    
    output(argv[2]);
    // show_mat(getDistAddr(0, 0, n), n);
    return 0;
}