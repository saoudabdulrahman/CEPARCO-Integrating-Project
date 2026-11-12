#include "grayscale.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

static double seconds_now(void)
{
#ifdef _WIN32
    static LARGE_INTEGER frequency = {0};
    LARGE_INTEGER counter;
    if (frequency.QuadPart == 0) QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

static int compare_doubles(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static void summarize(const double *samples, int runs, double *mean_ms, double *median_ms)
{
    double sorted[200], total = 0.0;
    int i;
    for (i = 0; i < runs; i++) {
        total += samples[i];
        sorted[i] = samples[i];
    }
    qsort(sorted, (size_t)runs, sizeof(double), compare_doubles);
    *mean_ms = (total / (double)runs) * 1000.0;
    if (runs % 2 == 0)
        *median_ms = (sorted[runs / 2 - 1] + sorted[runs / 2]) * 500.0;
    else
        *median_ms = sorted[runs / 2] * 1000.0;
}

/* Includes output copying and scratch allocation, but excludes file I/O. */
static int full_conversion(int mode, const BMP *bmp, unsigned char *output)
{
    memcpy(output, bmp->data, bmp->file_size);
    if (mode == 0) {
        grayscale_c(bmp, output);
        return 1;
    }
    if (mode == 1) return grayscale_planar(bmp, output);
    return grayscale_planar_avx2(bmp, output);
}

int benchmark_bmp(const char *filename, int runs)
{
    enum { WARMUPS = 5, FULL_MODES = 3, KERNEL_MODES = 2 };
    const char *full_names[3] = {"Sequential C", "Planar C", "Planar NASM AVX2"};
    const char *kernel_names[2] = {"Planar scalar C", "Planar NASM AVX2 + tail"};
    BMP bmp;
    unsigned char *reference = NULL, *output = NULL;
    float *red = NULL, *green = NULL, *blue = NULL;
    unsigned char *gray_c = NULL, *gray_avx2 = NULL;
    size_t count, float_bytes, m;
    double full_samples[3][200] = {{0}};
    double kernel_samples[2][200] = {{0}};
    double mean[3], median[3], kmean[2], kmedian[2];
    double begin, end;
    int pass, i, mode, ok = 0;

    if (runs < 30 || runs > 200) {
        printf("Benchmark repetitions must be from 30 to 200.\n");
        return 0;
    }
    if (!load_bmp(filename, &bmp)) return 0;
    if ((size_t)bmp.height > SIZE_MAX / (size_t)bmp.width) {
        printf("Image dimensions too large.\n");
        goto cleanup;
    }
    count = (size_t)bmp.width * (size_t)bmp.height;
    if (count > SIZE_MAX / sizeof(float) || count > SIZE_MAX / 13) {
        printf("Image dimensions too large.\n");
        goto cleanup;
    }
    float_bytes = count * sizeof(float);
    reference = (unsigned char *)malloc(bmp.file_size);
    output = (unsigned char *)malloc(bmp.file_size);
    if (!reference || !output) {
        printf("Not enough memory for benchmark output buffers.\n");
        goto cleanup;
    }

    
    if (!full_conversion(0, &bmp, reference)) goto cleanup;
    for (mode = 1; mode < FULL_MODES; mode++) {
        if (!full_conversion(mode, &bmp, output)) goto cleanup;
        if (memcmp(reference, output, bmp.file_size) != 0) {
            printf("FAIL: %s differs from sequential C output.\n", full_names[mode]);
            goto cleanup;
        }
    }

    /* Rotate run order between implementations. */
    for (pass = 0; pass < WARMUPS + runs; pass++) {
        for (i = 0; i < FULL_MODES; i++) {
            mode = (i + pass) % FULL_MODES;
            begin = seconds_now();
            if (!full_conversion(mode, &bmp, output)) goto cleanup;
            end = seconds_now();
            if (pass >= WARMUPS) full_samples[mode][pass - WARMUPS] = end - begin;
        }
    }

    printf("Benchmark: %d x %d (%zu pixels); runs=%d, warmups=%d\n",
           bmp.width, bmp.height, count, runs, WARMUPS);
    printf("PRIMARY: complete in-memory conversion, milliseconds\n");
    printf("Includes packed input -> complete 24-bit BGR output, copy + conversion + scratch allocations.\n");
    printf("Disk I/O and benchmark buffer allocations excluded.\n");
    printf("%-24s %14s %14s %12s\n", "Mode", "Mean(ms)", "Median(ms)", "Speedup");
    for (mode = 0; mode < FULL_MODES; mode++)
        summarize(full_samples[mode], runs, &mean[mode], &median[mode]);
    for (mode = 0; mode < FULL_MODES; mode++)
        printf("%-24s %14.6f %14.6f %11.3fx\n", full_names[mode],
               mean[mode], median[mode], mean[mode] ? mean[0] / mean[mode] : 0.0);

    
    red = (float *)malloc(float_bytes);
    green = (float *)malloc(float_bytes);
    blue = (float *)malloc(float_bytes);
    gray_c = (unsigned char *)malloc(count);
    gray_avx2 = (unsigned char *)malloc(count);
    if (!red || !green || !blue || !gray_c || !gray_avx2) {
        printf("Not enough memory for kernel-only benchmark buffers.\n");
        goto cleanup;
    }
    extract_channels(&bmp, red, green, blue);
    planar_scalar_byte_kernel(red, green, blue, gray_c, count);
    planar_avx2_byte_kernel(red, green, blue, gray_avx2, count);
    if (memcmp(gray_c, gray_avx2, count) != 0) {
        printf("FAIL: planar C and AVX2 kernel-only results differ.\n");
        goto cleanup;
    }
    for (pass = 0; pass < WARMUPS + runs; pass++) {
        for (i = 0; i < KERNEL_MODES; i++) {
            mode = (i + pass) % KERNEL_MODES;
            begin = seconds_now();
            if (mode == 0) planar_scalar_byte_kernel(red, green, blue, gray_c, count);
            else planar_avx2_byte_kernel(red, green, blue, gray_avx2, count);
            end = seconds_now();
            if (pass >= WARMUPS) kernel_samples[mode][pass - WARMUPS] = end - begin;
        }
    }
    printf("SECONDARY: kernel-only, prepared float channels -> grayscale bytes\n");
    printf("Excludes channel extraction, BGR repacking, and allocation.\n");
    printf("%-26s %14s %14s %12s\n", "Mode", "Mean(ms)", "Median(ms)", "Speedup");
    for (mode = 0; mode < KERNEL_MODES; mode++)
        summarize(kernel_samples[mode], runs, &kmean[mode], &kmedian[mode]);
    for (mode = 0; mode < KERNEL_MODES; mode++)
        printf("%-26s %14.6f %14.6f %11.3fx\n", kernel_names[mode],
               kmean[mode], kmedian[mode], kmean[mode] ? kmean[0] / kmean[mode] : 0.0);

    printf("Additional per-conversion temporary image storage (excluding input/output BMP):\n");
    printf("  Sequential C: 0 bytes\n");
    printf("  Planar C: %zu bytes (12 bytes/pixel)\n", count * 12);
    printf("  Planar NASM AVX2: %zu bytes (13 bytes/pixel)\n", count * 13);
    printf("  Note: benchmark-only arrays not included in per-conversion figures.\n");
    printf("PASS: all full-output and kernel-output equality checks.\n");
    printf("CSV_FULL,mode,mean_ms,median_ms,speedup_vs_sequential\n");
    for (m = 0; m < FULL_MODES; m++)
        printf("CSV_FULL,%s,%.9f,%.9f,%.9f\n", full_names[m],
               mean[m], median[m], mean[m] ? mean[0]/mean[m] : 0.0);
    printf("CSV_KERNEL,mode,mean_ms,median_ms,speedup_vs_planar_scalar\n");
    for (m = 0; m < KERNEL_MODES; m++)
        printf("CSV_KERNEL,%s,%.9f,%.9f,%.9f\n", kernel_names[m],
               kmean[m], kmedian[m], kmean[m] ? kmean[0]/kmean[m] : 0.0);
    printf("CSV_SAMPLE,comparison,mode,iteration,time_ms\n");
    for (mode = 0; mode < FULL_MODES; mode++)
        for (i = 0; i < runs; i++)
            printf("CSV_SAMPLE,complete_in_memory,%s,%d,%.9f\n",
                   full_names[mode], i + 1, full_samples[mode][i] * 1000.0);
    for (mode = 0; mode < KERNEL_MODES; mode++)
        for (i = 0; i < runs; i++)
            printf("CSV_SAMPLE,kernel_only_prepared,%s,%d,%.9f\n",
                   kernel_names[mode], i + 1, kernel_samples[mode][i] * 1000.0);
    ok = 1;
cleanup:
    free(red); free(green); free(blue);
    free(gray_c); free(gray_avx2);
    free(reference); free(output); free(bmp.data);
    return ok;
}
