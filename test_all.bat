@echo off
setlocal
cd /d "%~dp0"
if not exist grayscale.exe (
    echo grayscale.exe not found. Run build.bat first.
    exit /b 1
)
python tests\test_grayscale.py grayscale.exe
if errorlevel 1 exit /b 1
python tests\test_planar.py grayscale.exe
if errorlevel 1 exit /b 1
python tests\test_avx2.py grayscale.exe
if errorlevel 1 exit /b 1
python tests\test_benchmark.py grayscale.exe
if errorlevel 1 exit /b 1
python tests\test_extract.py grayscale.exe
if errorlevel 1 exit /b 1
echo All Week 10 test suites passed.
