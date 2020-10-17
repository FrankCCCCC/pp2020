#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
// #include <iostream>
#include <vector>
#include <mpi.h>
#include <algorithm>


void show_arr(float *a, int n){
    for(int i = 0; i < n; i++){
        printf("%f ", a[i]);
    }
    printf("\n");
}

// int rank: Current process rank
// int c_size: COMM_WORLD size
// int n: length of sequence
int count_b_size(int rank, int c_size, int n){
    // n / c_size
    int q = n / c_size;
    int r = n % c_size;
    if(r == 0){
        // Divisiable
        return q;
    }else{
        // handle residual
        if(rank < r){return q + 1;}
        else{return q;}
    }
}

// int rank: Current process rank
// int c_size: COMM_WORLD size
// int n: length of sequence
int count_b_start(int rank, int c_size, int n){
    // n / c_size
    int q = n / c_size;
    int r = n % c_size;
    if(r == 0){
        // Divisiable
        return q * rank;
    }else{
        // handle residual
        if(rank < r){return q * rank + rank;}
        else{return q * rank + r;}
    }
}

// if a > b, then return True
int cmp(const float *a, const float *b){
    if(*a > *b){return 1;}
    else{return 0;}
}
// if a > b, then return True for C++ sort
int cmp_plus(const float a, const float b){
    if(a < b){return 1;}
    else{return 0;}
}

// Merge and Reallocate
// float *buf: The buf that current process has
// int b_size: the size of buf
// float *buf_t: The buf that receive from other process
// int b_t_size: The size of buf_t
// int in_pair_id: The id of element in a pair, the left one is 0 and the right one is 1
float* merge_realc(float *buf, int b_size, float *buf_t, int b_t_size, int in_pair_id){
    int b_m_size = b_size;
    float *buf_m = (float*)malloc(sizeof(float) * b_m_size);
    int i = 0, b_i = 0, t_i = 0;

    if(!in_pair_id){
        // If in_pair_id == 0, redirect the array to smaller segment.
        for(i = 0, b_i = 0, t_i = 0; i < b_m_size && b_i < b_size && t_i < b_t_size;){
            if(buf_t[t_i] > buf[b_i]){buf_m[i++] = buf[b_i++];}
            else{buf_m[i++] = buf_t[t_i++];}
        }

        while(b_i < b_size && i < b_m_size){buf_m[i++] = buf[b_i++];}

        while(t_i < b_t_size && i < b_m_size){buf_m[i++] = buf_t[t_i++];}
    }else{
        // If in_pair_id == 1, redirect the array to larger segment.
        for(i = b_m_size - 1, b_i = b_size - 1, t_i = b_t_size - 1; i >= 0 && b_i >= 0 && t_i >= 0;){
            if(buf[b_i] > buf_t[t_i]){buf_m[i--] = buf[b_i--];}
            else{buf_m[i--] = buf_t[t_i--];}
        }
        while(b_i >= 0 && i >= 0){buf_m[i--] = buf[b_i--];}

        while(t_i >= 0 && i >= 0){buf_m[i--] = buf_t[t_i--];}
    }

    return buf_m;
}

// float* gather(int rank, float * buf, int b_size, int n, int c_size){
//     int max_buf_size = count_b_size(0, c_size, n);
//     float *res_t = (float*)malloc(sizeof(float) * max_buf_size * c_size);
//     float *res = (float*)malloc(sizeof(float) * n);
    
//     MPI_Gather(buf, b_size, MPI_FLOAT, res_t, max_buf_size, MPI_FLOAT, 0, MPI_COMM_WORLD);
    
//     if(rank == 0){
//         printf("Rank %d res_t: %f %f %f %f\n", rank, res_t[0], res_t[1], res_t[2], res_t[3]);
//         int res_i = 0;
//         for(int i = 0; i < c_size; i++){
//             int buf_i_size = count_b_size(i, c_size, n);
//             int buf_i_start = max_buf_size * i;
//             for(int j = 0; j < buf_i_size; j++){
//                 printf("Res[%d]: %f <<= res_t[%d]: %f\n", res_i, res[res_i], buf_i_start + j, res_t[buf_i_start + j]);
//                 res[res_i++] = res_t[buf_i_start + j];
//             }
//         }
//     }
//     free(res_t);

//     return res;
// }

float* gather(int rank, float *buf, int b_size, int n, int c_size){
    float *res = (float*)malloc(sizeof(float) * n);
    
    if(rank == 0){
        // Receive
        MPI_Request *req_arr = (MPI_Request*)malloc(sizeof(MPI_Request) * c_size);
        for(int i = 1; i < c_size; i++){
            int recv_start = count_b_start(i, c_size, n);
            int recv_size = count_b_size(i, c_size, n);
            MPI_Irecv(&(res[recv_start]), recv_size, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &(req_arr[i]));
        }

        for(int i = 1; i < c_size; i++){MPI_Wait(&(req_arr[i]), MPI_STATUS_IGNORE);}
        for(int i = 0; i < b_size; i++){res[i] = buf[i];}
        free(req_arr);
    }else{
        // Send
        MPI_Request req_s;
        MPI_Isend(buf, b_size, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &req_s);
    }

    return res;
}

// int rank: Current process rank
// int c_size: COMM_WORLD size
// float *buf: array of the sequence for corresponding  segment
// int b_size: buf size
// int n: length of sequence
float* OESort(int rank, int c_size, float *buf, int b_size, int n){
    // qsort(buf, b_size, sizeof(float), (int (*)(const void *, const void *))cmp);
    std::sort(buf, &(buf[b_size]));
    // printf("rank %d has array[%d]: %f %f\n", rank, b_size, buf[0], buf[1]);
    // show_arr(buf, b_size);

    for(int phase = 0; phase < c_size; phase++){
        // Check whether the target rank exist or not
        int target_rank = -1, in_pair_id = 0;
        if(rank % 2 == phase % 2){
            // Even Rank in Even Phase & Odd Rank in Odd Phase
            target_rank = (rank + 1) < c_size? (rank + 1) : -1;
            // Left one in a pair
            in_pair_id = 0;
        }else{
            // Odd Rank in Even Phase & Even Rank in Odd Phase
            target_rank = (rank - 1) >= 0? (rank - 1) : -1;
            // Right one in a pair
            in_pair_id = 1;
        }
        if(target_rank == -1){continue;}

        // Phase Sorting
        MPI_Request req_s, req_r;
        int b_t_size = count_b_size(target_rank, c_size, n);
        float *buf_t = (float*)malloc(sizeof(float) * b_t_size);

        MPI_Isend(buf, b_size, MPI_FLOAT, target_rank, in_pair_id, MPI_COMM_WORLD, &req_s);
        MPI_Irecv(buf_t, b_t_size, MPI_FLOAT, target_rank, (!in_pair_id), MPI_COMM_WORLD, &req_r);
        
        MPI_Wait(&req_r, MPI_STATUS_IGNORE);
        // printf("Phase %d rank %d(as id %d, TR %d) has reccived[%d]: %f %f\n", 
        //         phase, rank, in_pair_id, target_rank, b_t_size, buf_t[0], buf_t[1]);

        float *buf_m = merge_realc(buf, b_size, buf_t, b_t_size, in_pair_id);
        float *buf_d = buf;
        free(buf_d);
        buf = buf_m;

        // printf("Phase %d rank %d(as id %d, TR %d) merged[%d]: %f %f\n", 
        //         phase, rank, in_pair_id, target_rank, b_size, buf[0], buf[1]);
    }

    // printf("Rank %d array[%d]: %f %f\n", rank, b_size, buf[0], buf[1]);
    // MPI_Barrier(MPI_COMM_WORLD);
    // float *res = gather(rank, buf, b_size, n, c_size);
    // printf("Rank %d Res: %f %f %f %f\n", rank, res[0], res[1], res[2], res[3]);
    // if(rank == 0){show_arr(res, n);}

    return buf;
}

int main(int argc, char** argv) {
	MPI_Init(&argc,&argv);
	int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = atoi(argv[1]);
    int data_start = count_b_start(rank, size, n), 
        data_size = count_b_size(rank, size, n);
    float *data = (float*)malloc(sizeof(float) * n);
	
	MPI_File in_f, out_f;
	MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &in_f);
    MPI_File_open(MPI_COMM_WORLD, argv[3], MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &out_f);
	MPI_File_read_at(in_f, sizeof(float) * data_start, data, data_size, MPI_FLOAT, MPI_STATUS_IGNORE);

    // n = 8;
    // float data_t[8] = {8, 4, 6, 0, 3, 0, 0, 0};
    // for(int i = 0, j = data_start; i < data_size; i++, j++){
    //     data[i] = data_t[j];
    // }

    // printf("[1]%s, [2]%s, [3]%s\n", argv[1], argv[2], argv[3]);
    // printf("rank %d got float: \n", rank);
    // show_arr(data, data_size);
	
    // OESort(rank, size, &(data_t[count_b_start(rank, size, n)]), count_b_size(rank, size, n), n);
    data = OESort(rank, size, data, data_size, n);
    // printf("Rank %d Out %f\n", rank, data[0]);
    // show_arr(data, data_size);

	MPI_File_write_at(out_f, sizeof(float) * data_start, data, data_size, MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&in_f);
    MPI_File_close(&out_f);
	MPI_Finalize();
}