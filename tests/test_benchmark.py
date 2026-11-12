"""Verify benchmark output, correctness, and reported memory usage."""
import os
import subprocess
import sys
import tempfile
from test_planar import create_random_bmp


def main():
    if len(sys.argv) != 2:
        print('Usage: python tests\\test_benchmark.py grayscale.exe')
        return 1
    exe = os.path.abspath(sys.argv[1])
    with tempfile.TemporaryDirectory() as folder:
        path = os.path.join(folder, 'oddwidth.bmp')
        create_random_bmp(path, 13, -7, 1234)
        r = subprocess.run([exe, '--bench', path, '30'], check=True,
                           capture_output=True, text=True)
        result = r.stdout
        assert 'PASS: all full-output and kernel-output equality checks.' in result, result
        assert result.count('CSV_FULL,') == 4, result
        assert result.count('CSV_KERNEL,') == 3, result
        assert 'Planar C: 1092 bytes' in result, result  # 13*7*12
        assert 'Planar NASM AVX2: 1183 bytes' in result, result  # 13*7*13
    print('PASS: benchmark CLI, 30-run measurements, equality checks, and memory report.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
