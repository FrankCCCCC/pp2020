#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <assert.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <mpi.h>

#define CHUNK_SIZE 128

// Global Variables of Mandlebrot Set Calculation
int cpu_num = 0;
int iters = 0;
double left = 0;
double right = 0;
double lower = 0;
double upper = 0;
int width = 0;
int height = 0;
int area = 0;
int *image = NULL;
double *x0s = NULL;
double *y0s = NULL;

// Global Variables of Writing PNG 
int row_size = 0;
png_bytep raw_img = NULL;

// Vectorize Constant
const int vec_scale = 1;
const int vec_gap = 1 << vec_scale;
const double zero_vec[2] = {0};
const double one_vec[2] = {1};
const double two_vec[2] = {2, 2};
const double four_vec[2] = {4, 4};
double iters_vec[2] = {0};

const __m128d zerov = _mm_loadu_pd(zero_vec);
const __m128d onev = _mm_loadu_pd(one_vec);
const __m128d twov = _mm_loadu_pd(two_vec);
const __m128d fourv = _mm_loadu_pd(four_vec);
const __m128d fullv = _mm_or_pd(onev, onev);
__m128d itersv = _mm_loadu_pd(zero_vec);

void assign_iters_vec(int iters){
    iters_vec[0] = (double)iters; iters_vec[1] = (double)iters;
    itersv = _mm_loadu_pd(iters_vec);
}

void assign_x0s_y0s(){
	for (int j = 0; j < height; ++j) {
        double y0 = j * ((upper - lower) / height) + lower;
        for (int i = 0; i < width; ++i) {
            double x0 = i * ((right - left) / width) + left;

            int serial_id = j * width + i;
            x0s[serial_id] = x0;
            y0s[serial_id] = y0;
        }
    }
}

void png_write_sig(int x, int y){
    int p = image[(height - 1 - y) * width + x];
    png_bytep color = &(raw_img[row_size * y]) + x * 3;
    if (p != iters) {
        if (p & 16) {
            color[0] = 240;
            color[1] = color[2] = p % 16 * 16;
        } else {
            color[0] = p % 16 * 16;
        }
    }
}

void image_to_png(int ps, int size){
    for(int i = ps; i < ps + size; i++){
        int x = i % width;
        int y = (height - 1 - (i / width));
        png_write_sig(x, y);
    }
}

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
    
    for (int y = 0; y < height; ++y) {
        png_write_row(png_ptr, &(raw_img[row_size * y]));
    }
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

void core_cal_sse2_sig(double *x0, double *y0, int *image){
    __m128d repeatsv = _mm_loadu_pd(zero_vec);
    __m128d xv = _mm_loadu_pd(zero_vec);
    __m128d yv = _mm_loadu_pd(zero_vec);
    __m128d length_squaredv = _mm_loadu_pd(zero_vec);

    const __m128d x0v = _mm_loadu_pd(x0);
    const __m128d y0v = _mm_loadu_pd(y0);

    while(1){
        __m128d compv = _mm_and_pd(_mm_cmplt_pd(repeatsv, itersv), _mm_cmplt_pd(length_squaredv, fourv));
        compv = (__m128d)_mm_slli_epi64((__m128i)compv, 54);
        compv = (__m128d)_mm_srli_epi64((__m128i)compv, 2);

        unsigned long int comp_vec[2] = {0, 0};
        _mm_store_pd((double*)comp_vec, compv);
        // printf("Compv0: %lu %lu, Compv: %lu %lu\n", comp_vec0[0], comp_vec0[1], comp_vec[0], comp_vec[1]);
        if((comp_vec[0] == 0) && (comp_vec[1] == 0)){break;}
        
        __m128d tempv = zerov;
        tempv = _mm_add_pd(_mm_sub_pd(_mm_mul_pd(xv, xv), _mm_mul_pd(yv, yv)), x0v) ;
        yv = _mm_add_pd(_mm_mul_pd(twov, _mm_mul_pd(xv, yv)), y0v);
        xv = tempv;
        length_squaredv = _mm_add_pd(_mm_mul_pd(xv, xv), _mm_mul_pd(yv, yv));

        repeatsv = _mm_add_pd(repeatsv, compv);
    }   
    double image_temp[2] = {0, 0};
    _mm_store_pd(image_temp, repeatsv);
    image[0] = (int)(image_temp[0]);
    image[1] = (int)(image_temp[1]);

    // printf("Iters %d, (%d, %d) <- (%lf, %lf)\n", count, image[i], image[i + 1], image_temp[0], image_temp[1]);
}

void core_cal(double x0, double y0, int *image){
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

void core_cal_sse2(int ps, int size){
    const int size_vec = (size >> vec_scale) << vec_scale;
    const int max_loop_vec = ps + size_vec;
    const int max_loop = ps + size;
    for(int i = ps; i < max_loop_vec; i+=vec_gap){
        core_cal_sse2_sig(&(x0s[i]), &(y0s[i]), &(image[i]));
    }

    for(int i = max_loop_vec; i < max_loop; i++){
        core_cal(x0s[i], y0s[i], &(image[i]));
    }

    image_to_png(ps, size);
}

int main(int argc, char** argv) {
	MPI_Init(&argc,&argv);
	int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	/* argument parsing */
    assert(argc == 9);
    const char* filename = argv[1];
    iters = strtol(argv[2], 0, 10);
    left = strtod(argv[3], 0);
    right = strtod(argv[4], 0);
    lower = strtod(argv[5], 0);
    upper = strtod(argv[6], 0);
    width = strtol(argv[7], 0, 10);
    height = strtol(argv[8], 0, 10);
    area = width * height;
    assign_iters_vec(iters);

    /* allocate memory for image */
    image = (int*)malloc(area * sizeof(int));
    // memset(image, 0, area * sizeof(int));
    assert(image);

    // allocate for png
    row_size = 3 * width * sizeof(png_byte);
    raw_img = (png_bytep)malloc(row_size * height);

    // Allocate for x0, y0
    x0s = (double*)malloc(sizeof(double) * area);
    y0s = (double*)malloc(sizeof(double) * area);

    /* mandelbrot set */
    assign_x0s_y0s();

	// Task_Queue *tq = create_queue(area / CHUNK_SIZE + 1);
    // int idx = 0;
    // for(idx = 0; idx + CHUNK_SIZE < area; idx+=CHUNK_SIZE){
    //     make_task(idx, CHUNK_SIZE, tq);
    // }
    // make_task(idx, area - idx, tq);

    
    MPI_Finalize();
}