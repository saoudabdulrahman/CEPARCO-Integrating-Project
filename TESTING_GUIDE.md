# Week 10 — Windows build and testing guide

Run these steps in order. All commands assume you are in the **repository root** (the folder containing `build.bat`). Do not use the 32-bit/x86 Developer Command Prompt.

## A. Prepare the command prompt

1. Extract the ZIP. Open **x64 Native Tools Command Prompt for VS 2022**.
2. Navigate into the extracted project (change the path if you saved it elsewhere):

Example
   ```bat
   cd /d "C:\Users\Francis Medin\xxxxxx\xxxxxx"
   ```

3. Check the tools:

   ```bat
   cl
   nasm -v
   python --version
   ```

   `cl` without files prints its version and usage information. `nasm -v` should print a version. If Python isn't recognized, try `py --version` and replace `python` with `py` in later commands.

## B. Compile the project

```bat
build.bat
```

The build assembles both `.asm` files as Win64 object files and compiles five C files. The expected final line is:

```text
Build successful: grayscale.exe
```

You can also build manually:

```bat
nasm -f win64 src\planar_avx2.asm -o planar_avx2.obj
nasm -f win64 src\packed_extract.asm -o packed_extract.obj
cl /nologo /O2 /fp:precise /Qvec-report:2 src\main.c src\bmp.c src\grayscale.c src\benchmark.c src\packed_check.c planar_avx2.obj packed_extract.obj /Fe:grayscale.exe
```

Compiler messages beginning `info C5002` describe vectorization decisions; they are **not errors**. Record them to support the scalar-reference discussion. `LNK1112` mentioning x86/x64 means the wrong Developer Command Prompt was opened.

## C. Run all correctness and benchmark-framework tests

```bat
test_all.bat
```

For individual test results, run:

```bat
python tests\test_grayscale.py grayscale.exe
python tests\test_planar.py grayscale.exe
python tests\test_avx2.py grayscale.exe
python tests\test_benchmark.py grayscale.exe
python tests\test_extract.py grayscale.exe
```

Expected messages:

```text
PASS: 14 BMP test cases; padding, orientation, and pixel colors verified.
PASS: 44 cases; planar-float C output matches sequential C byte-for-byte.
PASS: 90 cases; NASM planar AVX2 output matches sequential C byte-for-byte.
PASS: benchmark CLI, 30-run measurements, equality checks, and memory report.
PASS: 78 BMP extraction cases plus guarded boundary load.
```

The 14 + 44 + 90 + 78 = **226** correctness cases are not 226 independent images; they are counts across the named test suites. The guarded-boundary check and benchmark-framework check are additional tests. The extract tests do not test a completed direct packed-BMP grayscale kernel.

If a test fails, stop there and retain the full command output. The previous Stage 4A build passed these suites, but **results from the reorganized version must be confirmed on your Windows system**.

## D. Confirm the grayscale image output yourself

Generate sample BMPs:

```bat
python tests\create_bench_inputs.py
```

Create three grayscale images:

```bat
grayscale.exe benchmark_inputs\640x480.bmp demo_sequential.bmp
grayscale.exe --planar benchmark_inputs\640x480.bmp demo_planar_c.bmp
grayscale.exe --avx2 benchmark_inputs\640x480.bmp demo_planar_avx2.bmp
```

Compare the output files byte-for-byte in Command Prompt:

```bat
fc /b demo_sequential.bmp demo_planar_c.bmp
fc /b demo_sequential.bmp demo_planar_avx2.bmp
```

Both comparisons should say **`FC: no differences encountered`**. Open each image to verify that it appears grayscale. The test suites provide more systematic checks than this one demonstration image.

## E. Benchmark one image

```bat
grayscale.exe --bench benchmark_inputs\640x480.bmp 30
```

Look for the two tables:

- **PRIMARY: complete in-memory conversion** includes processing from packed pixels to a complete grayscale BMP output buffer, with copying and scratch allocations, but not disk I/O.
- **SECONDARY: kernel-only** uses prepared float channels and excludes channel extraction, repacking, and allocations.

The program should also print memory usage and `PASS: all full-output and kernel-output equality checks.` Do not compare the secondary 35–63× historical ratios directly with application-level speedups.

## F. Benchmark all five resolutions

With the test input images already generated, run:

```bat
python tests\collect_benchmarks.py grayscale.exe 30 results\benchmark_results_new.csv
```

Expected final lines:

```text
Saved 25 summary rows to results\benchmark_results_new.csv
Saved 750 individual timings to results\benchmark_results_new_samples.csv
```

The measurements are for 640×480, 1280×720, 1920×1080, 3840×2160, and 641×479. Keep the two newly generated CSVs; use them to update the report only after checking the values. For more reliable timings, close heavy background applications, use a stable power setting and repeat complete benchmark sessions. Do not replace previous results silently; label the run date and environment.

## G. Record evidence for the report

Record your exact CPU model, RAM, OS version, MSVC version, NASM version and date of measurement. In PowerShell:

```powershell
Get-CimInstance Win32_Processor | Select-Object Name
Get-CimInstance Win32_ComputerSystem | Select-Object TotalPhysicalMemory
```

Also capture the terminal output from `build.bat`, `test_all.bat`, and the benchmark, and save the new CSVs. Document how the submitted NASM kernels were developed by group members and disclose every relevant use of AI. The current assembly files are AI-assisted instructional examples until your group independently implements its submitted versions.

## Troubleshooting

| Symptom | Check |
|---|---|
| `'nasm' is not recognized` | Confirm NASM is installed and on PATH. |
| Device Guard blocks `nasm.exe` | Check Windows Security policy; seek an approved install rather than bypassing organizational restrictions. |
| `LNK1112 ... x86 ... x64` | Use the **x64** Native Tools prompt. |
| `main.c` or `src\...` not found | Run commands from the project root. |
| `C5002` reports | Informational messages, not fatal errors. |
| A Python test fails | Read the error and verify the build was successful; do not proceed to performance claims. |
| `grayscale.exe` not recognized | Run the command from the project root, or use `.\grayscale.exe`. |
| Benchmark times fluctuate | Repeat runs; record mean and median, avoid background workloads. |
