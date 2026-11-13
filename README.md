# CEPARCO Integrating Project — Group 7

## Performance Analysis of Planar-Float and Direct Packed-BMP AVX2 Implementations for Image Grayscale Conversion

**Course:** CEPARCO — Integrating Project  
**Group members:** Abdulrahman, Lim, Lopez, Nicolas  
**Current milestone:** Week 10 — Mid-Project

## Overview

This project studies grayscale conversion of uncompressed 24-bit BMP images using sequential C and SIMD implementations written in NASM for x86-64 AVX2. The study evaluates whether the performance benefits of vectorized grayscale computation outweigh the costs of extracting image channels, allocating temporary storage, and preparing the output.

The Week 10 implementation includes:

- A sequential C grayscale reference implementation.
- A planar-float C grayscale implementation.
- A planar-float NASM AVX2 grayscale kernel that processes eight pixels per vector iteration, with remaining pixels handled in C.
- An initial direct packed-BGR NASM AVX2 prototype that extracts the channels of eight packed 24-bit pixels. **This prototype does not yet perform complete grayscale conversion.**

The primary performance metric is **complete in-memory conversion**, from packed BMP pixel data already loaded into memory to a grayscale BGR output buffer. Prepared-float **kernel-only** timing is reported separately and must not be interpreted as full application speedup. Disk file I/O is excluded from these measurements.

## Project Structure

```text
src/
  main.c                  Command-line interface and program entry point
  bmp.c                   BMP reading, validation, and writing
  grayscale.h             Shared definitions and function declarations
  grayscale.c             Sequential C, planar C, and AVX2 integration
  benchmark.c             Timing and temporary-memory measurements
  packed_check.c          Packed extraction and boundary-safety checks
  planar_avx2.asm         Eight-pixel planar-float grayscale kernel
  packed_extract.asm      Eight-pixel packed-BGR channel extraction prototype

tests/
  test_grayscale.py       BMP and sequential conversion tests
  test_planar.py          Planar C correctness tests
  test_avx2.py            Planar NASM AVX2 correctness tests
  test_benchmark.py       Benchmark interface and reporting tests
  test_extract.py         Packed extraction and boundary tests
  create_bench_inputs.py  Generate benchmark BMP inputs
  collect_benchmarks.py  Collect benchmark summary and sample CSVs

results/
  benchmark_results_new.csv
  benchmark_results_new_samples.csv

build.bat                 Build for Windows x64
test_all.bat             Run the five automated test suites
TESTING_GUIDE.md          Detailed setup, testing, and troubleshooting
```

## Grayscale Conversion Rule

Each 8-bit B, G, and R channel value is converted to single-precision floating point. The intended computation is:

```text
Y = ((0.299f * R) + (0.587f * G)) + (0.114f * B)
```

The result is clamped to the range `[0, 255]` and rounded to the nearest integer using ties-to-even rounding. The grayscale byte is written to all three B, G, and R output channels.

The supported input format is **uncompressed 24-bit Windows BMP**, including row padding and both positive-height and negative-height image layouts. The automated comparisons verify byte-for-byte agreement for the tested inputs; a complete proof of equivalence for all valid inputs remains future work.

## Requirements

- Windows x64 and an AVX2-capable processor
- Microsoft Visual Studio 2022 C/C++ Build Tools or Community edition
- **x64 Native Tools Command Prompt for VS 2022**
- NASM assembler
- Python 3

### Documented test environment

| Component | Configuration |
|---|---|
| Processor | AMD Ryzen 7 8845HS with Radeon 780M Graphics |
| Operating system | Windows 11 Home Single Language, build 26200.9974 |
| System-reported physical memory | 13.75 GB |
| C compiler | MSVC 19.44.35229 (x64) |
| Assembler | NASM 3.02 |
| Compiler options | `/O2 /fp:precise /Qvec-report:2` |
| Active Windows power plan | Balanced |

## Build and Run

Open **x64 Native Tools Command Prompt for VS 2022**, change to the repository directory, then run:

```bat
build.bat
```

A successful build ends with:

```text
Build successful: grayscale.exe
```

Compiler `C5001` and `C5002` messages are informational vectorization diagnostics, not build errors.

To create sample inputs and compare the implemented conversion modes:

```bat
python tests\create_bench_inputs.py
grayscale.exe benchmark_inputs\640x480.bmp demo_sequential.bmp
grayscale.exe --planar benchmark_inputs\640x480.bmp demo_planar_c.bmp
grayscale.exe --avx2 benchmark_inputs\640x480.bmp demo_planar_avx2.bmp
```

See [TESTING_GUIDE.md](TESTING_GUIDE.md) for additional examples.

## Correctness Testing

Run all five automated test suites:

```bat
test_all.bat
```

A recorded Windows x64 execution of the local Week 10 implementation produced:

```text
PASS: 14 BMP test cases; padding, orientation, and pixel colors verified.
PASS: 44 cases; planar-float C output matches sequential C byte-for-byte.
PASS: 90 cases; NASM planar AVX2 output matches sequential C byte-for-byte.
PASS: benchmark CLI, 30-run measurements, equality checks, and memory report.
PASS: 78 BMP extraction cases plus guarded boundary load.
All Week 10 test suites passed.
```

This corresponds to **226 correctness cases** across the four counted suites, plus benchmark-framework and protected-boundary checks. The counts represent test cases, not necessarily distinct image files. The protected-boundary test checks the implemented load scenario, not universal memory safety.

These results were obtained from the local Week 10 project folder. Re-run `build.bat` and `test_all.bat` on the exact final GitHub version before submission to confirm that the archived and submitted files produce the same results.

To save a test log:

```bat
test_all.bat > results\test_results.txt 2>&1
```

## Benchmarking

Benchmark one input image with 30 measured iterations:

```bat
grayscale.exe --bench benchmark_inputs\640x480.bmp 30
```

Collect measurements for the five benchmark resolutions:

```bat
python tests\collect_benchmarks.py grayscale.exe 30 results\benchmark_results_new.csv
```

The collector also generates `results\benchmark_results_new_samples.csv`.

The repository includes **25 benchmark summary rows** and **750 timing samples** from the recorded Windows session. Each configuration used five warm-up iterations and 30 timed iterations. The results cover five resolutions and distinguish:

1. **Complete in-memory conversion:** sequential C, planar C, and planar NASM AVX2, including applicable channel extraction, scratch allocations, grayscale processing, and output preparation.
2. **Prepared-float kernel-only computation:** scalar C and NASM AVX2 computation on already separated float arrays, excluding extraction, repacking, and allocation.

### Preliminary complete-conversion results

| Resolution | Sequential C mean (ms) | Planar C mean (ms) | Planar AVX2 mean (ms) | AVX2 speedup |
|---|---:|---:|---:|---:|
| 640 × 480 | 5.06 | 6.96 | 2.90 | 1.75× |
| 1280 × 720 | 15.66 | 21.97 | 9.61 | 1.63× |
| 1920 × 1080 | 34.24 | 42.29 | 15.36 | 2.23× |
| 3840 × 2160 | 138.28 | 172.12 | 66.50 | 2.08× |
| 641 × 479 | 4.93 | 6.38 | 2.40 | 2.06× |

Speedup is calculated as `sequential C time / planar AVX2 time` for the complete-conversion measurements. These preliminary results come from one benchmark session and are subject to timing variation. They **do not** represent a comparison with a completed direct packed-BMP grayscale implementation.

## Temporary Memory

For `N = image width × absolute image height`:

- Sequential C uses **0 additional image-sized scratch bytes**.
- Planar C uses **12N bytes** for three float channel arrays.
- Planar NASM AVX2 uses **13N bytes**, including the float arrays and an N-byte grayscale staging buffer.

These figures exclude shared BMP input/output buffers, allocation overhead, and benchmark-only data. The complete packed-BMP grayscale implementation's memory requirements remain to be measured.

## Week 10 Status and Week 13 Plan

**Completed and tested for the Week 10 prototype:**

- Reading, validating, and writing 24-bit BMP images.
- Sequential C and planar C grayscale conversion.
- Planar-float NASM AVX2 grayscale processing and C tail handling.
- Initial safe packed-BGR SIMD channel extraction.
- Automated correctness, benchmark-framework, and guarded-boundary tests.
- Preliminary complete-conversion and kernel-only benchmarks.

**Remaining for Week 13:**

- Complete the direct packed-BMP AVX2 grayscale conversion path.
- Verify identical arithmetic and output, including additional edge and memory-boundary cases.
- Benchmark all completed implementations using equivalent packed-input-to-packed-output boundaries.
- Repeat performance sessions under documented conditions and report variability and memory overhead.

## AI Assistance and Kernel Authorship

ChatGPT was used to help prepare minor initial C implementation examples, automated tests, suggested benchmarking code, debugging guidance, analysis, and minor documentation. The group compiled, executed, tested, and benchmarked the current project implementation on Windows x64.

**The current NASM kernels are prototypes.** Passing correctness tests confirms their observed behavior on tested inputs. The group independently implemented, reviewed, and documented its submitted assembly kernels.

## References

- [OpenCV — Color Conversions](https://docs.opencv.org/4.13.0/de/d25/imgproc_color_conversions.html)
- [Simd Image Processing Library](https://github.com/ermig1979/Simd)
- [Intel 64 and IA-32 Architectures Software Developer's Manuals](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [Microsoft Learn — BITMAPINFOHEADER](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader)

## Project Videos

- [Original video pitch](https://drive.google.com/file/d/1lUj4cKhRb9e4dU-IyHioNdkyP3rqXDsl/view)
- [Revised video pitch](https://drive.google.com/file/d/1oaG3sUWaKnIROWXJ6y1eRCfXm2TA-yw_/view?usp=sharing)
- [Week 10 demonstration video link](https://drive.google.com/file/d/1PzqQglrg3QWC3D0TMigBhStxQMGcD6Iy/view?usp=sharing)
