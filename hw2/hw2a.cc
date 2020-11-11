#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define PNG_NO_SETJMP
#include <sched.h>
#include <assert.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <xmmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

// #define FULL64 9223372036854775808

void write_png(const char* filename, int iters, int width, int height, const int* buffer) {
    FILE* fp = fopen(filename, "wb");
    assert(fp);
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    assert(png_ptr);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    assert(info_ptr);
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_filter(png_ptr, 0, PNG_NO_FILTERS);
    png_write_info(png_ptr, info_ptr);
    png_set_compression_level(png_ptr, 1);
    size_t row_size = 3 * width * sizeof(png_byte);
    png_bytep row = (png_bytep)malloc(row_size);
    for (int y = 0; y < height; ++y) {
        memset(row, 0, row_size);
        for (int x = 0; x < width; ++x) {
            int p = buffer[(height - 1 - y) * width + x];
            png_bytep color = row + x * 3;
            if (p != iters) {
                if (p & 16) {
                    color[0] = 240;
                    color[1] = color[2] = p % 16 * 16;
                } else {
                    color[0] = p % 16 * 16;
                }
            }
        }
        png_write_row(png_ptr, row);
    }
    free(row);
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

void core_cal_sse2(int iters, double *x0, double *y0, int *image, int size){
    const int vec_scale = 1;
    const int vec_gap = 1 << vec_scale;
    const int size_vec = (size >> vec_scale) << vec_scale;
    const double zero_vec[2] = {0, 0};
    const double one_vec[2] = {1, 1};
    const double two_vec[2] = {2, 2};
    const double four_vec[2] = {4, 4};
    const double iters_vec[2] = {(double)iters, (double)iters};
    // const unsigned long int full_vec[2] = {FULL64, FULL64};
    // const long int zero_li_vec[2] = {0, 0};
    const __m128d zerov = _mm_loadu_pd(zero_vec);
    const __m128d onev = _mm_loadu_pd(one_vec);
    const __m128d twov = _mm_loadu_pd(two_vec);
    const __m128d fourv = _mm_loadu_pd(four_vec);
    const __m128d itersv = _mm_loadu_pd(iters_vec);
    const __m128d fullv = _mm_or_pd(onev, onev);
    // const __m128i zeroliv = _mm_loadu_pd(&(zero_li_vec[0]));
    
    for(int i = 0; i < size_vec; i+=vec_gap){
        __m128d repeatsv = _mm_loadu_pd(zero_vec);
        __m128d xv = _mm_loadu_pd(zero_vec);
        __m128d yv = _mm_loadu_pd(zero_vec);
        __m128d length_squaredv = _mm_loadu_pd(&(zero_vec[i]));

        const __m128d x0v = _mm_loadu_pd(&(x0[i]));
        const __m128d y0v = _mm_loadu_pd(&(y0[i]));

        int count = 0;

        while(1){
            __m128d compv = _mm_and_pd(_mm_cmplt_pd(repeatsv, itersv), _mm_cmplt_pd(length_squaredv, fourv));
            // __m128d compv1 = _mm_andnot_pd(compv, fullv);
            compv = (__m128d)_mm_slli_epi64((__m128i)compv, 54);
            compv = (__m128d)_mm_srli_epi64((__m128i)compv, 2);

            unsigned long int comp_vec[2] = {0, 0};
            // unsigned long int comp_vec1[2] = {0, 0};
            _mm_store_pd((double*)comp_vec, compv);
            // _mm_store_pd((double*)comp_vec1, compv1);
            // printf("Compv0: %lu %lu, Compv: %lu %lu\n", comp_vec0[0], comp_vec0[1], comp_vec[0], comp_vec[1]);
            if((comp_vec[0] == 0) && (comp_vec[1] == 0)){break;}
            
            __m128d tempv = zerov;
            tempv = _mm_add_pd(_mm_sub_pd(_mm_mul_pd(xv, xv), _mm_mul_pd(yv, yv)), x0v) ;
            yv = _mm_add_pd(_mm_mul_pd(twov, _mm_mul_pd(xv, yv)), y0v);
            xv = tempv;
            length_squaredv = _mm_add_pd(_mm_mul_pd(xv, xv), _mm_mul_pd(yv, yv));

            repeatsv = _mm_add_pd(repeatsv, compv);

            if(count > iters){
                // printf("Shutdown\n");
                break;
            }
            else{count++;}
        }   
        double image_temp[2] = {0, 0};
        _mm_store_pd(image_temp, repeatsv);
        // for(int i = 0; i < vec_gap; i++){
        image[i] = (int)(image_temp[0]);
        image[i + 1] = (int)(image_temp[1]);
        // }
        // printf("Iters %d, (%d, %d) <- (%lf, %lf)\n", count, image[i], image[i + 1], image_temp[0], image_temp[1]);
    }

    for(int i = size_vec; i < size; i++){
        int repeats = 0;
        double x = 0;
        double y = 0;
        double length_squared = 0;
        while (repeats < iters && length_squared < 4) {
            double temp = x * x - y * y + x0[i];
            y = 2 * x * y + y0[i];
            x = temp;
            length_squared = x * x + y * y;
            ++repeats;
        }

        image[i] = repeats;
        // printf("Iters None, %d\n", image[i]);
    }
}

void core_cal_float(int iters, double x0, double y0, int *image){
    int repeats = 0;
    float x = 0;
    float y = 0;
    float length_squared = 0;
    while (repeats < iters && length_squared < 4) {
        float temp = x * x - y * y + (float)x0;
        y = 2 * x * y + (float)y0;
        x = temp;
        length_squared = x * x + y * y;
        ++repeats;
    }

    *image = repeats;
}

void core_cal(int iters, double x0, double y0, int *image){
    int repeats = 0;
    double x = 0;
    double y = 0;
    double length_squared = 0;
    while (repeats < iters && length_squared < 4) {
        double temp = x * x - y * y + x0;
        y = 2 * x * y + y0;
        x = temp;
        length_squared = x * x + y * y;
        ++repeats;
    }

    *image = repeats;
}

int main(int argc, char** argv) {
    /* detect how many CPUs are available */
    cpu_set_t cpu_set;
    sched_getaffinity(0, sizeof(cpu_set), &cpu_set);
    printf("%d cpus available\n", CPU_COUNT(&cpu_set));

    /* argument parsing */
    assert(argc == 9);
    const char* filename = argv[1];
    int iters = strtol(argv[2], 0, 10);
    double left = strtod(argv[3], 0);
    double right = strtod(argv[4], 0);
    double lower = strtod(argv[5], 0);
    double upper = strtod(argv[6], 0);
    int width = strtol(argv[7], 0, 10);
    int height = strtol(argv[8], 0, 10);

    /* allocate memory for image */
    int* image = (int*)malloc(width * height * sizeof(int));
    assert(image);

    // double *x0s = (double*)malloc(sizeof(double) * height * width);
    // double *y0s = (double*)malloc(sizeof(double) * height * width);
    /* mandelbrot set */
    for (int j = 0; j < height; ++j) {
        double y0 = j * ((upper - lower) / height) + lower;
        for (int i = 0; i < width; ++i) {
            double x0 = i * ((right - left) / width) + left;

            core_cal_float(iters, x0, y0, &(image[j * width + i]));
            // core_cal(iters, x0, y0, &(image[j * width + i]));
            // x0s[j * width + i] = x0;
            // y0s[j * width + i] = y0;
        }
    }
    // printf("x0s y0s Done\n");
    // core_cal_sse2(iters, x0s, y0s, image, height * width);
    // free(x0s);
    // free(y0s);

    /* draw and cleanup */
    write_png(filename, iters, width, height, image);
    free(image);
}
