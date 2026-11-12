#include "grayscale.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

int packed_extract_check(const char *filename)
{
    BMP bmp;
    size_t blocks = 0, tails = 0;
    int y, x, lane;
    float red[8], green[8], blue[8];

    if (!load_bmp(filename, &bmp)) return 0;
    for (y = 0; y < bmp.height; y++) {
        const unsigned char *row = bmp.data + bmp.pixel_offset +
                                   (size_t)y * bmp.row_stride;
        for (x = 0; x <= bmp.width - 8; x += 8) {
            const unsigned char *src = row + (size_t)x * 3;
            
            packed_bgr8_extract(src, red, green, blue);
            for (lane = 0; lane < 8; lane++) {
                size_t p = (size_t)lane * 3;
                if (blue[lane] != (float)src[p] ||
                    green[lane] != (float)src[p + 1] ||
                    red[lane] != (float)src[p + 2]) {
                    printf("FAIL: packed channel extraction at row %d, x=%d\n",
                           y, x + lane);
                    free(bmp.data);
                    return 0;
                }
            }
            blocks++;
        }
        
        tails += (size_t)(bmp.width - x);
    }
    printf("PASS: packed BGR extraction matches C; image=%dx%d; "
           "SIMD blocks=%zu (8 pixels each); scalar tails=%zu.\n",
           bmp.width, bmp.height, blocks, tails);
    printf("NOTE: extraction prototype only; no packed grayscale or packed benchmarks yet.\n");
    free(bmp.data);
    return 1;
}

#ifdef _WIN32
/* Detect reads past the end of an eight-pixel block. */
int packed_extract_guard_test(void)
{
    SYSTEM_INFO info;
    unsigned char *area, *end_block;
    DWORD old_protect;
    size_t p;
    float r[8], g[8], b[8];
    int i, ok = 1;
    GetSystemInfo(&info);
    p = (size_t)info.dwPageSize;
    area = (unsigned char *)VirtualAlloc(NULL, p * 2,
                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!area) { printf("FAIL: VirtualAlloc.\n"); return 0; }
    if (!VirtualProtect(area + p, p, PAGE_NOACCESS, &old_protect)) {
        printf("FAIL: VirtualProtect.\n");
        VirtualFree(area, 0, MEM_RELEASE);
        return 0;
    }
    end_block = area + p - 24;
    for (i = 0; i < 24; i++) end_block[i] = (unsigned char)((i * 37 + 11) & 255);
    packed_bgr8_extract(end_block, r, g, b);
    for (i = 0; i < 8; i++) {
        if (b[i] != (float)end_block[3*i] ||
            g[i] != (float)end_block[3*i+1] ||
            r[i] != (float)end_block[3*i+2]) {
            ok = 0;
            break;
        }
    }
    VirtualFree(area, 0, MEM_RELEASE);
    if (!ok) { printf("FAIL: guard-boundary channel mismatch.\n"); return 0; }
    printf("PASS: packed 24-byte load at a guarded page boundary.\n");
    return 1;
}
#else
/* Same boundary test for non-Windows builds. */
int packed_extract_guard_test(void)
{
    long page_size = sysconf(_SC_PAGESIZE);
    unsigned char *area, *end_block;
    size_t p;
    float r[8], g[8], b[8];
    int i, ok = 1;
    if (page_size < 24) return 0;
    p = (size_t)page_size;
    area = (unsigned char *)mmap(NULL, p * 2, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (area == MAP_FAILED) return 0;
    if (mprotect(area + p, p, PROT_NONE) != 0) {
        munmap(area, p * 2);
        return 0;
    }
    end_block = area + p - 24;
    for (i = 0; i < 24; i++) end_block[i] = (unsigned char)((i * 37 + 11) & 255);
    packed_bgr8_extract(end_block, r, g, b);
    for (i = 0; i < 8; i++) {
        if (b[i] != (float)end_block[3*i] ||
            g[i] != (float)end_block[3*i+1] ||
            r[i] != (float)end_block[3*i+2]) {
            ok = 0;
            break;
        }
    }
    munmap(area, p * 2);
    if (!ok) return 0;
    printf("PASS: packed 24-byte load at a guarded page boundary.\n");
    return 1;
}
#endif
