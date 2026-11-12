@echo off
setlocal
cd /d "%~dp0"
nasm -f win64 src\planar_avx2.asm -o planar_avx2.obj
if errorlevel 1 exit /b 1
nasm -f win64 src\packed_extract.asm -o packed_extract.obj
if errorlevel 1 exit /b 1
cl /nologo /O2 /fp:precise /Qvec-report:2 src\main.c src\bmp.c src\grayscale.c src\benchmark.c src\packed_check.c planar_avx2.obj packed_extract.obj /Fe:grayscale.exe
if errorlevel 1 exit /b 1
echo Build successful: grayscale.exe
