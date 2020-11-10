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
#include <xmmintrin.h>
#include <emmintrin.h>

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

void core_cal_vec(int iters, double *x0, double *y0, int *image, int size){
    int vec_scale = 1;
    int vec_gap = 1 << vec_scale;
    int size_vec = (size >> vec_scale) << vec_scale;
    const double zero_vec[2] = {0, 0};
    const double two_vec[2] = {2, 2};
    const __m128d zerov = _mm_loadu_pd(&(zero_vec[0]));
    const __m128d twov = _mm_loadu_pd(&(two_vec[0]));

    for(int i = 0; i < size_vec; i+=vec_gap){
        __m128d x0v = _mm_loadu_pd(&(x0[i]));
        __m128d y0v = _mm_loadu_pd(&(y0[i]));

        __m128d xv = _mm_loadu_pd(&(zero_vec[0]));
        __m128d yv = _mm_loadu_pd(&(zero_vec[0]));
        __m128d length_squaredv = _mm_loadu_pd(&(zero_vec[0]));
        while(1){
            __m128d tempv = zerov;
            tempv = _mm_add_pd(_mm_sub_pd(_mm_mul_pd(xv, xv), _mm_mul_pd(yv, yv)), x0v) ;
            yv = _mm_add_pd(_mm_mul_pd(twov, _mm_mul_pd(xv, yv)), y0v);
            xv = tempv;
            length_squaredv = _mm_add_pd(_mm_mul_pd(xv, xv), _mm_mul_pd(yv, yv));
        }   
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

        *image = repeats;
    }
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

    /* mandelbrot set */
    for (int j = 0; j < height; ++j) {
        double y0 = j * ((upper - lower) / height) + lower;
        for (int i = 0; i < width; ++i) {
            double x0 = i * ((right - left) / width) + left;

            core_cal(iters, x0, y0, &(image[j * width + i]));
            // int repeats = 0;
            // double x = 0;
            // double y = 0;
            // double length_squared = 0;
            // while (repeats < iters && length_squared < 4) {
            //     double temp = x * x - y * y + x0;
            //     y = 2 * x * y + y0;
            //     x = temp;
            //     length_squared = x * x + y * y;
            //     ++repeats;
            // }
            // image[j * width + i] = repeats;


        }
    }

    /* draw and cleanup */
    write_png(filename, iters, width, height, image);
    free(image);
}
