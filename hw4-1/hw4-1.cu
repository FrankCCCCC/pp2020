#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <cuda.h>

#define SIZEOFINT sizeof(int)

#define TH_DIM 32
const dim3 thread_dim(TH_DIM, TH_DIM);
const int block_num = 5000;

const int INF = ((1 << 30) - 1);
const int V = 4000;
// void show_mat(int *, int);
// void malloc_Dist(int);
// void setup_DistCuda(int);
// int getDist(int ,int, int);
// int *getDistAddr(int ,int, int);
// void setDist(int ,int, int, int);
// void input(char* inFileName);
// void output(char* outFileName);

// void block_FW_cuda(int B);
// __global__ void cal_cuda(int *Dist, int vertex_num, int edge_num, int B, int Round, int block_start_x, int block_start_y, int block_width, int block_height);
// void block_FW(int B);
// int ceil(int a, int b);
// void cal(int vertex_num, int edge_num, int B, int Round, int block_start_x, int block_start_y, int block_width, int block_height);

const int B = 32;
int n, m;
int *Dist;
int *Dist_cuda;

void show_mat(int *start_p, int vertex_num){
    for(int i = 0; i < vertex_num; i++){
        for(int j = 0; j < vertex_num; j++){
            if(start_p[i * vertex_num + j] == INF){
                printf("INF\t  ");
            }else{
                printf("%d\t  ", start_p[i * vertex_num + j]);
            }
            
        }
        printf("\n");
    }
}

void malloc_Dist(int vertex_num){Dist = (int*)malloc(SIZEOFINT * vertex_num * vertex_num);}
int getDist(int i, int j, int vertex_num){return Dist[i * vertex_num + j];}
int *getDistAddr(int i, int j, int vertex_num){return &(Dist[i * vertex_num + j]);}
void setDist(int i, int j, int val, int vertex_num){Dist[i * vertex_num + j] = val;}

void setup_DistCuda(int vertex_num){
    cudaMalloc((void **)&Dist_cuda, SIZEOFINT * vertex_num * vertex_num);
    cudaMemcpy(Dist_cuda, Dist, (n * n * SIZEOFINT), cudaMemcpyHostToDevice);
}
void back_DistCuda(int vertex_num){
    cudaMemcpy(Dist, Dist_cuda, (n * n * SIZEOFINT), cudaMemcpyDeviceToHost);
}
// int getDistCuda(int i, int j, int vertex_num){return Dist_cuda[i * vertex_num + j];}
// int *getDistAddrCuda(int i, int j, int vertex_num){return &(Dist_cuda[i * vertex_num + j]);}
// void setDistCuda(int i, int j, int val, int vertex_num){Dist_cuda[i * vertex_num + j] = val;}

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

__global__ void cal_cuda(int *dist, int vertex_num, int edge_num, int B, int Round, int block_start_x, int block_start_y, int block_width, int block_height) {
    int block_end_x = block_start_x + block_height;
    int block_end_y = block_start_y + block_width;
    __shared__ int s_m[100][100];
    // printf("%d\n", dist[1]);

    for (int b_i = block_start_x; b_i < block_end_x; b_i++) {
        for (int b_j = block_start_y; b_j < block_end_y; b_j++) {
            // To calculate B*B elements in the block (b_i, b_j)
            // For each block, it need to compute B times

            // for (int i = block_internal_start_x + threadIdx.x; i < block_internal_end_x; i+=blockDim.x) {
            //     for (int j = block_internal_start_y + threadIdx.y; j < block_internal_end_y; j+=blockDim.y) {
                    
            //     }
            // }

            for (int k = Round * B; k < (Round + 1) * B && k < vertex_num; k++) {
                // To calculate original index of elements in the block (b_i, b_j)
                // For instance, original index of (0,0) in block (1,2) is (2,5) for V=6,B=2
                int block_internal_start_x = b_i * B;
                int block_internal_end_x = (b_i + 1) * B;
                int block_internal_start_y = b_j * B;
                int block_internal_end_y = (b_j + 1) * B;

                if (block_internal_end_x > vertex_num) block_internal_end_x = vertex_num;
                if (block_internal_end_y > vertex_num) block_internal_end_y = vertex_num;

                for (int i = block_internal_start_x + threadIdx.x; i < block_internal_end_x; i+=blockDim.x) {
                    for (int j = block_internal_start_y + threadIdx.y; j < block_internal_end_y; j+=blockDim.y) {
                        int d = dist[i * vertex_num + k] + dist[k * vertex_num + j];
                        if (d < dist[i * vertex_num + j]) {
                            dist[i * vertex_num + j] = d;
                        }
                        // dist[i * vertex_num + j] = 2;
                    }
                }
                __syncthreads();
            }
        }
    }
}

void block_FW_cuda(int B) {
    int round = (n + B - 1) / B;
    for (int r = 0; r < round; r++) {
        printf("Round: %d in total: %d\n", r, round);
        fflush(stdout);
        /* Phase 1*/
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, r, r, 1, 1);

        /* Phase 2*/
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, r, 0, r, 1);
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, r, r + 1, round - r - 1, 1);
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, 0, r, 1, r);
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, r + 1, r, 1, round - r - 1);

        /* Phase 3*/
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, 0, 0, r, r);
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, 0, r + 1, round - r - 1, r);
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, r + 1, 0, r, round - r - 1);
        cal_cuda<<<1, thread_dim>>>(Dist_cuda, n, m, B, r, r + 1, r + 1, round - r - 1, round - r - 1);
    }
}

void cal(int vertex_num, int edge_num, int B, int Round, int block_start_x, int block_start_y, int block_width, int block_height) {
    int block_end_x = block_start_x + block_height;
    int block_end_y = block_start_y + block_width;

    for (int b_i = block_start_x; b_i < block_end_x; b_i++) {
        for (int b_j = block_start_y; b_j < block_end_y; b_j++) {
            // To calculate B*B elements in the block (b_i, b_j)
            // For each block, it need to compute B times            

            for (int k = Round * B; k < (Round + 1) * B && k < vertex_num; k++) {
                // To calculate original index of elements in the block (b_i, b_j)
                // For instance, original index of (0,0) in block (1,2) is (2,5) for V=6,B=2
                int block_internal_start_x = b_i * B;
                int block_internal_end_x = (b_i + 1) * B;
                int block_internal_start_y = b_j * B;
                int block_internal_end_y = (b_j + 1) * B;

                if (block_internal_end_x > vertex_num) block_internal_end_x = vertex_num;
                if (block_internal_end_y > vertex_num) block_internal_end_y = vertex_num;

                for (int i = block_internal_start_x; i < block_internal_end_x; i++) {
                    for (int j = block_internal_start_y; j < block_internal_end_y; j++) {
                        // if (Dist[i][k] + Dist[k][j] < Dist[i][j]) {
                        //     Dist[i][j] = Dist[i][k] + Dist[k][j];
                        // }
                        if (getDist(i, k, vertex_num) + getDist(k, j, vertex_num) < getDist(i, j, vertex_num)) {
                            setDist(i, j, getDist(i, k, vertex_num) + getDist(k, j, vertex_num), vertex_num);
                        }
                    }
                }
            }
        }
    }
}

int ceil(int a, int b) { return (a + b - 1) / b; }

void block_FW(int B) {
    int round = ceil(n, B);
    for (int r = 0; r < round; r++) {
        printf("Round: %d in total: %d\n", r, round);
        fflush(stdout);
        /* Phase 1*/
        cal(n, m, B, r, r, r, 1, 1);

        /* Phase 2*/
        cal(n, m, B, r, r, 0, r, 1);
        cal(n, m, B, r, r, r + 1, round - r - 1, 1);
        cal(n, m, B, r, 0, r, 1, r);
        cal(n, m, B, r, r + 1, r, 1, round - r - 1);

        /* Phase 3*/
        cal(n, m, B, r, 0, 0, r, r);
        cal(n, m, B, r, 0, r + 1, round - r - 1, r);
        cal(n, m, B, r, r + 1, 0, r, round - r - 1);
        cal(n, m, B, r, r + 1, r + 1, round - r - 1, round - r - 1);
    }
}



int main(int argc, char* argv[]) {
    input(argv[1]);
    // show_mat(getDistAddr(0, 0, n), n);
    setup_DistCuda(n);
    printf("Vertice: %d, Edge: %d\n", n, m);
    // block_FW(B);
    block_FW_cuda(B);
    back_DistCuda(n);
    // show_mat(getDistAddr(0, 0, n), n);
    
    output(argv[2]);
    // show_mat(getDistAddr(0, 0, n), n);
    return 0;
}