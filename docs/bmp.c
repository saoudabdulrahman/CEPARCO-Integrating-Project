#include "grayscale.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

/* BMP header fields are little-endian. */
static uint16_t read_u16(const unsigned char *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int load_bmp(const char *filename, BMP *bmp)
{
    FILE *fp = fopen(filename, "rb");
    long length;
    uint32_t dib_size, offset;
    int32_t width, signed_height;
    uint64_t stride, pixels_size;

    memset(bmp, 0, sizeof(*bmp));

    if (fp == NULL) {
        printf("Cannot open input file: %s\n", filename);
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0 || (length = ftell(fp)) < 54 ||
        fseek(fp, 0, SEEK_SET) != 0) {
        printf("Invalid or unreadable BMP file.\n");
        fclose(fp);
        return 0;
    }

    bmp->file_size = (size_t)length;
    bmp->data = (unsigned char *)malloc(bmp->file_size);
    if (bmp->data == NULL) {
        printf("Not enough memory.\n");
        fclose(fp);
        return 0;
    }

    if (fread(bmp->data, 1, bmp->file_size, fp) != bmp->file_size) {
        printf("Could not read the complete BMP file.\n");
        fclose(fp);
        free(bmp->data);
        bmp->data = NULL;
        return 0;
    }
    fclose(fp);

    
    dib_size = read_u32(bmp->data + 14);
    offset = read_u32(bmp->data + 10);
    width = (int32_t)read_u32(bmp->data + 18);
    signed_height = (int32_t)read_u32(bmp->data + 22);

    if (bmp->data[0] != 'B' || bmp->data[1] != 'M' ||
        dib_size < 40 || (uint64_t)14 + dib_size > bmp->file_size ||
        (uint64_t)offset < (uint64_t)14 + dib_size || offset > bmp->file_size ||
        read_u16(bmp->data + 26) != 1 ||
        read_u16(bmp->data + 28) != 24 ||
        read_u32(bmp->data + 30) != 0 ||
        width <= 0 || signed_height == 0 || signed_height == INT32_MIN) {
        printf("Unsupported BMP: expected uncompressed 24-bit Windows BMP.\n");
        free(bmp->data);
        bmp->data = NULL;
        return 0;
    }

    bmp->width = width;
    bmp->height = (signed_height < 0) ? -signed_height : signed_height;
    bmp->pixel_offset = offset;

    /* Each row is aligned to four bytes. */
    stride = ((uint64_t)bmp->width * 3 + 3) / 4 * 4;
    pixels_size = stride * (uint64_t)bmp->height;
    if (stride > SIZE_MAX || pixels_size > bmp->file_size - bmp->pixel_offset) {
        printf("BMP pixel data is truncated or has invalid dimensions.\n");
        free(bmp->data);
        bmp->data = NULL;
        return 0;
    }
    bmp->row_stride = (size_t)stride;
    return 1;
}

int save_file(const char *filename, const unsigned char *data, size_t size)
{
    FILE *fp = fopen(filename, "wb");
    int ok;
    if (fp == NULL) return 0;
    ok = (fwrite(data, 1, size, fp) == size);
    if (fclose(fp) != 0) ok = 0;
    return ok;
}
