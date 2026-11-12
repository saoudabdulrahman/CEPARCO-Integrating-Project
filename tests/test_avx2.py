"""Stage 3: compare NASM planar AVX2 output byte-for-byte to sequential C."""
import os
import subprocess
import sys
import tempfile
from test_planar import create_random_bmp
from test_grayscale import create_bmp


def compare(program, folder, width, signed_height, seed=None):
    source = os.path.join(folder, 'input.bmp')
    reference = os.path.join(folder, 'reference.bmp')
    avx2 = os.path.join(folder, 'avx2.bmp')
    if seed is None:
        create_bmp(source, width, signed_height)
    else:
        create_random_bmp(source, width, signed_height, seed)

    subprocess.run([program, source, reference], check=True, capture_output=True)
    subprocess.run([program, '--avx2', source, avx2], check=True, capture_output=True)

    with open(reference, 'rb') as f:
        expected = f.read()
    with open(avx2, 'rb') as f:
        actual = f.read()
    if expected != actual:
        first = next((i for i, (a, b) in enumerate(zip(expected, actual)) if a != b), None)
        if first is None:
            first = min(len(expected), len(actual))
        raise AssertionError(
            f'MISMATCH: {width=} {signed_height=} {seed=} at file offset {first}; '
            f'expected={expected[first:first+8].hex()} actual={actual[first:first+8].hex()}')


def main():
    if len(sys.argv) != 2:
        print('Usage: python tests\\test_avx2.py grayscale.exe')
        return 1
    program = os.path.abspath(sys.argv[1])
    cases = 0
    with tempfile.TemporaryDirectory() as folder:
        for width in (1, 2, 3, 7, 8, 9, 13, 15, 16, 17, 31, 32, 33, 640):
            for height in (1, -1, 3, -3):
                compare(program, folder, width, height)
                cases += 1
        for i, width in enumerate((1, 2, 7, 8, 9, 13, 17, 23, 31, 32, 33, 65, 127, 128, 129, 257, 640)):
            for height in (5, -5):
                compare(program, folder, width, height, seed=2000+i)
                cases += 1
    print(f'PASS: {cases} cases; NASM planar AVX2 output matches sequential C byte-for-byte.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
