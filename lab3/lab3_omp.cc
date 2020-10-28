#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>

unsigned long long *res = NULL;

int ncpus = 0;
unsigned long long r = 0;
unsigned long long k = 0;
unsigned long long r_squrt = 0;

unsigned long long calc_pixs(int rank){
    unsigned long long pixels = 0;
    for (unsigned long long x = (unsigned long long)rank; x < r; x+=(unsigned long long)ncpus) {
		unsigned long long y = ceil(sqrtl(r_squrt - x*x));
		pixels += y;
		pixels %= k;
	}
    return pixels;
}

void task(int rk){
    res[rk] = calc_pixs(rk);
}

int main(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, "must provide exactly 2 arguments!\n");
		return 1;
	}

	r = atoll(argv[1]);
	k = atoll(argv[2]);
    r_squrt = r * r;
	unsigned long long pixels = 0;
    res = (unsigned long long*)malloc(sizeof(unsigned long long) * ncpus);
    // ranks = (unsigned long long*)malloc(sizeof(unsigned long long) * ncpus);

    int omp_threads, omp_thread;

    #pragma omp parallel
    {
        
        ncpus = omp_get_num_threads();
        int rank = omp_get_thread_num();
        task(rank);
    }

	for(int i = 0; i < ncpus; i++){
        pixels += res[i];
		pixels %= k;
    }
    free(res);
    // free(ranks);

	// for (unsigned long long x = 0; x < r; x++) {
	// 	unsigned long long y = ceil(sqrtl(r*r - x*x));
	// 	pixels += y;
	// 	pixels %= k;
	// }
	printf("%llu\n", (4 * pixels) % k);
}
