#ifndef GRAYSCALE_H
#define GRAYSCALE_H

#include <stddef.h>

typedef struct {
    unsigned char *data;
    size_t file_size;
    size_t pixel_offset;
    size_t row_stride;
    int width;
    int height;
} BMP;

int load_bmp(const char *filename, BMP *bmp);
int save_file(const char *filename, const unsigned char *data, size_t size);
void grayscale_c(const BMP *bmp, unsigned char *output);
int grayscale_planar(const BMP *bmp, unsigned char *output);
int grayscale_planar_avx2(const BMP *bmp, unsigned char *output);
void extract_channels(const BMP *bmp, float *red, float *green, float *blue);
unsigned char grayscale_byte(float value);
void planar_scalar_byte_kernel(const float *red, const float *green,
                               const float *blue, unsigned char *gray, size_t count);
void planar_avx2_byte_kernel(const float *red, const float *green,
                             const float *blue, unsigned char *gray, size_t count);
int benchmark_bmp(const char *filename, int runs);
int packed_extract_check(const char *filename);
int packed_extract_guard_test(void);

/* NASM entry points. */
void planar_avx2_kernel(const float *red, const float *green,
                        const float *blue, unsigned char *gray, size_t count);
void packed_bgr8_extract(const unsigned char *bgr24, float *red8,
                         float *green8, float *blue8);

#endif
