#include "grayscale.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Do not auto-vectorize the scalar reference loops. */
#if defined(_MSC_VER)
#define NO_AUTO_VECTOR __pragma(loop(no_vector))
#define NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#define NO_AUTO_VECTOR
#define NOINLINE __attribute__((noinline))
#else
#define NO_AUTO_VECTOR
#define NOINLINE
#endif

/* Round to nearest integer; halfway values round to even. */
unsigned char grayscale_byte(float value)
{
    int whole;
    float fractional;

    if (value <= 0.0f) return 0;
    if (value >= 255.0f) return 255;

    whole = (int)value;
    fractional = value - (float)whole;
    if (fractional > 0.5f ||
        (fractional == 0.5f && (whole % 2) != 0)) {
        whole++;
    }
    return (unsigned char)whole;
}

void grayscale_c(const BMP *bmp, unsigned char *output)
{
    int y, x;
    for (y = 0; y < bmp->height; y++) {
        size_t row_start = bmp->pixel_offset + (size_t)y * bmp->row_stride;
        NO_AUTO_VECTOR
        for (x = 0; x < bmp->width; x++) {
            size_t p = row_start + (size_t)x * 3;
            float blue = (float)bmp->data[p];
            float green = (float)bmp->data[p + 1];
            float red = (float)bmp->data[p + 2];
            float value = (0.299f * red + 0.587f * green) + 0.114f * blue;
            unsigned char gray = grayscale_byte(value);

            output[p] = gray;
            output[p + 1] = gray;
            output[p + 2] = gray;
        }
    }
}

void extract_channels(const BMP *bmp,
                             float *red, float *green, float *blue)
{
    int y, x;
    for (y = 0; y < bmp->height; y++) {
        size_t row_start = bmp->pixel_offset + (size_t)y * bmp->row_stride;
        for (x = 0; x < bmp->width; x++) {
            size_t p = row_start + (size_t)x * 3;
            size_t i = (size_t)y * (size_t)bmp->width + (size_t)x;

            blue[i] = (float)bmp->data[p];
            green[i] = (float)bmp->data[p + 1];
            red[i] = (float)bmp->data[p + 2];
        }
    }
}

static void grayscale_planar_c(const BMP *bmp, unsigned char *output,
                               const float *red, const float *green,
                               const float *blue)
{
    int y, x;
    for (y = 0; y < bmp->height; y++) {
        size_t row_start = bmp->pixel_offset + (size_t)y * bmp->row_stride;
        NO_AUTO_VECTOR
        for (x = 0; x < bmp->width; x++) {
            size_t i = (size_t)y * (size_t)bmp->width + (size_t)x;
            size_t p = row_start + (size_t)x * 3;
            float value = (0.299f * red[i] + 0.587f * green[i])
                          + 0.114f * blue[i];
            unsigned char gray = grayscale_byte(value);

            output[p] = gray;
            output[p + 1] = gray;
            output[p + 2] = gray;
        }
    }
}

int grayscale_planar(const BMP *bmp, unsigned char *output)
{
    float *red, *green, *blue;
    size_t count, bytes;

    if ((size_t)bmp->height > SIZE_MAX / (size_t)bmp->width) {
        printf("Image dimensions are too large.\n");
        return 0;
    }
    count = (size_t)bmp->width * (size_t)bmp->height;
    if (count > SIZE_MAX / sizeof(float)) {
        printf("Not enough address space for planar arrays.\n");
        return 0;
    }
    bytes = count * sizeof(float);

    red = (float *)malloc(bytes);
    green = (float *)malloc(bytes);
    blue = (float *)malloc(bytes);
    if (red == NULL || green == NULL || blue == NULL) {
        printf("Not enough memory for planar arrays.\n");
        free(red);
        free(green);
        free(blue);
        return 0;
    }

    extract_channels(bmp, red, green, blue);
    grayscale_planar_c(bmp, output, red, green, blue);

    free(red);
    free(green);
    free(blue);
    return 1;
}

int grayscale_planar_avx2(const BMP *bmp, unsigned char *output)
{
    float *red = NULL, *green = NULL, *blue = NULL;
    unsigned char *gray = NULL;
    size_t count, float_bytes, i;
    int y, x;

    if ((size_t)bmp->height > SIZE_MAX / (size_t)bmp->width) {
        printf("Image dimensions are too large.\n");
        return 0;
    }
    count = (size_t)bmp->width * (size_t)bmp->height;
    if (count > SIZE_MAX / sizeof(float)) {
        printf("Not enough address space for planar arrays.\n");
        return 0;
    }
    float_bytes = count * sizeof(float);

    red = (float *)malloc(float_bytes);
    green = (float *)malloc(float_bytes);
    blue = (float *)malloc(float_bytes);
    gray = (unsigned char *)malloc(count);
    if (red == NULL || green == NULL || blue == NULL || gray == NULL) {
        printf("Not enough memory for planar AVX2 buffers.\n");
        free(red);
        free(green);
        free(blue);
        free(gray);
        return 0;
    }

    extract_channels(bmp, red, green, blue);
    planar_avx2_kernel(red, green, blue, gray, count);

    /* Handle the remaining pixels after full AVX2 blocks. */
    for (i = (count / 8) * 8; i < count; i++) {
        float value = (0.299f * red[i] + 0.587f * green[i])
                      + 0.114f * blue[i];
        gray[i] = grayscale_byte(value);
    }

    
    for (y = 0; y < bmp->height; y++) {
        size_t row_start = bmp->pixel_offset + (size_t)y * bmp->row_stride;
        for (x = 0; x < bmp->width; x++) {
            size_t pixel = (size_t)y * (size_t)bmp->width + (size_t)x;
            size_t p = row_start + (size_t)x * 3;
            output[p] = gray[pixel];
            output[p + 1] = gray[pixel];
            output[p + 2] = gray[pixel];
        }
    }

    free(red);
    free(green);
    free(blue);
    free(gray);
    return 1;
}

NOINLINE void planar_scalar_byte_kernel(const float *red, const float *green,
                                               const float *blue, unsigned char *gray,
                                               size_t count)
{
    size_t i;
    NO_AUTO_VECTOR
    for (i = 0; i < count; i++) {
        float value = (0.299f * red[i] + 0.587f * green[i])
                      + 0.114f * blue[i];
        gray[i] = grayscale_byte(value);
    }
}

NOINLINE void planar_avx2_byte_kernel(const float *red, const float *green,
                                            const float *blue, unsigned char *gray,
                                            size_t count)
{
    size_t i;
    planar_avx2_kernel(red, green, blue, gray, count);
    for (i = (count / 8) * 8; i < count; i++) {
        float value = (0.299f * red[i] + 0.587f * green[i])
                      + 0.114f * blue[i];
        gray[i] = grayscale_byte(value);
    }
}
