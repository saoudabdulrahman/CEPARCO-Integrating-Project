#include "grayscale.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    BMP bmp;
    unsigned char *output;
    const char *input_name, *output_name;
    int planar_mode = 0;
    int avx2_mode = 0;
    int ok;

    if (argc == 2 && strcmp(argv[1], "--extract-guard") == 0)
        return packed_extract_guard_test() ? 0 : 1;
    if (argc == 3 && strcmp(argv[1], "--extract-check") == 0)
        return packed_extract_check(argv[2]) ? 0 : 1;

    if ((argc == 3 || argc == 4) && strcmp(argv[1], "--bench") == 0) {
        int runs = 30;
        if (argc == 4) {
            char *end = NULL;
            long parsed = strtol(argv[3], &end, 10);
            if (!end || *end != '\0' || parsed < 30 || parsed > 200) {
                printf("Usage: grayscale --bench input.bmp [runs: 30..200]\n");
                return 1;
            }
            runs = (int)parsed;
        }
        return benchmark_bmp(argv[2], runs) ? 0 : 1;
    }

    if (argc == 3) {
        input_name = argv[1];
        output_name = argv[2];
    } else if (argc == 4 && strcmp(argv[1], "--planar") == 0) {
        planar_mode = 1;
        input_name = argv[2];
        output_name = argv[3];
    } else if (argc == 4 && strcmp(argv[1], "--avx2") == 0) {
        avx2_mode = 1;
        input_name = argv[2];
        output_name = argv[3];
    } else {
        printf("Usage: grayscale input.bmp output.bmp\n");
        printf("   or: grayscale --planar input.bmp output.bmp\n");
        printf("   or: grayscale --avx2 input.bmp output.bmp\n");
        printf("   or: grayscale --bench input.bmp [runs: 30..200]\n");
        printf("   or: grayscale --extract-check input.bmp\n");
        printf("   or: grayscale --extract-guard\n");
        return 1;
    }
    if (strcmp(input_name, output_name) == 0) {
        printf("Input and output files must have different names.\n");
        return 1;
    }
    if (!load_bmp(input_name, &bmp)) return 1;

    output = (unsigned char *)malloc(bmp.file_size);
    if (output == NULL) {
        printf("Not enough memory for output image.\n");
        free(bmp.data);
        return 1;
    }

    memcpy(output, bmp.data, bmp.file_size);
    if (avx2_mode) {
        ok = grayscale_planar_avx2(&bmp, output);
    } else if (planar_mode) {
        ok = grayscale_planar(&bmp, output);
    } else {
        grayscale_c(&bmp, output);
        ok = 1;
    }
    if (ok) ok = save_file(output_name, output, bmp.file_size);

    if (ok) {
        printf("Grayscale BMP created: %s\n", output_name);
        printf("Mode: %s\n", avx2_mode ? "planar-float NASM AVX2" :
               (planar_mode ? "planar-float C (not AVX2)" : "sequential C"));
        printf("Image: %d x %d; row stride: %zu bytes\n",
               bmp.width, bmp.height, bmp.row_stride);
    } else {
        printf("Conversion failed or output could not be saved.\n");
    }

    free(output);
    free(bmp.data);
    return ok ? 0 : 1;
}
