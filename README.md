# CEPARCO-Integrating-Project
# Group 7 — Grayscale Conversion with AVX2

**Project:** Performance Analysis of Planar-Float and Direct Packed-BMP AVX2 Implementations for Image Grayscale Conversion  
**Group:** Abdulrahman, Lim, Lopez, Nicolas  
**Milestone:** Week 10 (mid-project)

## Overview

This project compares grayscale conversion using sequential C, planar-float C, and a planar-float NASM AVX2 kernel. It also contains an **extraction-only** AVX2 prototype for eight packed 24-bit BMP pixels. The complete direct packed-BMP grayscale kernel is planned for Week 13.

The main comparison is **complete in-memory conversion** from packed BMP data to a grayscale BMP buffer. A separate prepared-float **kernel-only** comparison is included, but it is not an application-level speedup measurement.

## Files

```text
src/
  grayscale.h         Shared BMP definition and function declarations
  main.c              Command-line options and conversion entry point
  bmp.c               BMP input validation and output writing
  grayscale.c         Sequential C, planar C, and planar AVX2 integration
  benchmark.c         Timing, speedup, and temporary-memory reporting
  packed_check.c      BGR extraction checks and guarded-boundary test
  planar_avx2.asm     Planar-float eight-pixel grayscale kernel
  packed_extract.asm  Packed-BGR eight-pixel channel extraction only
tests/                 Five test suites and two input/benchmark utilities
results/               Historical Stage 3B Windows benchmark CSV files
docs/                  Proposal, report draft, benchmark notes, disclosures
build.bat              Windows x64 build
test_all.bat           Run all five tests
TESTING_GUIDE.md       Step-by-step build, test and benchmark instructions
```

The numerical rule is `((0.299f * R) + (0.587f * G)) + (0.114f * B)`, followed by clamping to `[0,255]` and rounding to nearest integer (ties to even). Output pixels repeat the resulting byte in the B, G and R channels. Input files must be uncompressed 24-bit Windows BMP files.

## Requirements

- Windows x64 and a CPU supporting AVX2
- Visual Studio 2022 C/C++ compiler; use **x64 Native Tools Command Prompt**
- NASM (previously tested with NASM 3.02)
- Python 3 for the tests and input generation

## Build and test

Open the x64 Native Tools Command Prompt, navigate to this folder and execute:

```bat
build.bat
test_all.bat
```

Five test suites are expected to report **14, 44, 90 and 78 passing cases**, plus a benchmark-framework check and a protected-page boundary check. These counts were observed in the earlier Stage 4A Windows build. **This reorganized source must be rebuilt and retested on Windows before submission.**

To convert a BMP image:

```bat
python tests\create_bench_inputs.py
grayscale.exe benchmark_inputs\640x480.bmp demo_sequential.bmp
grayscale.exe --planar benchmark_inputs\640x480.bmp demo_planar_c.bmp
grayscale.exe --avx2 benchmark_inputs\640x480.bmp demo_planar_avx2.bmp
```

For full testing, example outputs, troubleshooting and benchmarking, follow [TESTING_GUIDE.md](TESTING_GUIDE.md).

## Performance evaluation

Run a single image benchmark:

```bat
grayscale.exe --bench benchmark_inputs\640x480.bmp 30
```

Reproduce all five resolutions and write CSV files:

```bat
python tests\collect_benchmarks.py grayscale.exe 30 results\benchmark_results_new.csv
```

The collector also writes `results\benchmark_results_new_samples.csv`. The included files `benchmark_results.csv` and `benchmark_results_samples.csv` contain **earlier Stage 3B Windows measurements, not new measurements from this refactor**. Re-run the collector before reporting results for this version. Refer to `docs/Benchmark_Notes.md` for the comparison boundaries and memory accounting.

## Current status and remaining work

- Sequential C, planar C and planar NASM AVX2: working in the previously tested Stage 4A version; revalidation of the reorganized build is pending.
- Packed-BGR AVX2: extracts eight BGR pixels into separate float channels; **does not convert the image to grayscale yet**.
- The five resolution benchmarks compare sequential C, planar C and planar AVX2; they do **not** compare the unfinished direct packed-BMP grayscale implementation.
- Week 13 work: complete the direct packed-BMP kernel, verify all outputs and memory boundaries, repeat the full benchmarks and report final memory costs.

## References

Background reading: [OpenCV color conversions](https://docs.opencv.org/4.13.0/de/d25/imgproc_color_conversions.html), [Simd Library](https://github.com/ermig1979/Simd), [Intel software developer manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html), and [Microsoft BMP documentation](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader).



## Video pitch: https://drive.google.com/file/d/1lUj4cKhRb9e4dU-IyHioNdkyP3rqXDsl/view
## Video pitch (revisions): https://drive.google.com/file/d/1oaG3sUWaKnIROWXJ6y1eRCfXm2TA-yw_/view?usp=sharing
## Week 10 Demo Video:
