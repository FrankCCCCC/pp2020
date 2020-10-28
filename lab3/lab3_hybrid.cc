#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <mpi.h>
#include <omp.h>

#define Root2 1.414213562373095

unsigned long long *res = NULL;

int ncpus = 0;
unsigned long long r = 0;
unsigned long long k = 0;
unsigned long long r_squr = 0;
unsigned long long r_half = 0;

unsigned long long calc_pixs_m(int rank){
    unsigned long long pixels = 0;
    for (unsigned long long x = (unsigned long long)rank; x < r_half; x+=(unsigned long long)ncpus) {
		unsigned long long y = ceil(sqrtl(r_squr - x*x)) - r_half;
        // printf("Rank %d x: %llu, y: %llu\n", rank, x, y);
		pixels += y;
		pixels %= k;
	}
    // printf("Rank %d pixels: %llu\n", rank, pixels);
    return pixels;
}

void task(int rk){
    res[rk] = calc_pixs_m(rk);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int mpi_rank, mpi_ranks, omp_threads, omp_thread;
    char hostname[HOST_NAME_MAX];

    assert(!gethostname(hostname, HOST_NAME_MAX));
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_ranks);

    r = atoll(argv[1]);
	k = atoll(argv[2]);
    r_squr = r * r;
    r_half = ceil((double)r / (double)Root2);

    unsigned long long pixels = 0;
    unsigned long long pixels_sum = 0;
    res = (unsigned long long*)malloc(sizeof(unsigned long long) * 1000);

#pragma omp parallel
    {
        omp_threads = omp_get_num_threads();
        omp_thread = omp_get_thread_num();
        ncpus = mpi_ranks * omp_threads;
        printf("Hello %s: rank %2d/%2d, thread %2d/%2d\n", hostname, mpi_rank, mpi_ranks,
               omp_thread, omp_threads);
        task(mpi_rank * omp_threads + omp_thread);
    }

    for(int i = 0; i < omp_threads; i++){
        pixels += res[i];
		pixels %= k;
    }
    pixels = ((pixels * 2) % k) + ((r_half * r_half) % k);
    pixels %= k;

    MPI_Reduce(&pixels, &pixels_sum, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    pixels_sum += pixels;
    free(res);

    printf("%llu\n", (4 * pixels_sum) % k);

    MPI_Finalize();
}
