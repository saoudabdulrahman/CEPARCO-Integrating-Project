"""Compare grayscale output from sequential C and planar-float C."""
import os
import random
import struct
import subprocess
import sys
import tempfile

from test_grayscale import create_bmp


def create_random_bmp(path, width, signed_height, seed):
    """Build a BMP with reproducible random BGR pixels and padding."""
    rng = random.Random(seed)
    height = abs(signed_height)
    stride = ((width * 3 + 3) // 4) * 4
    pixels = bytearray()
    for _ in range(height):
        for _ in range(width * 3):
            pixels.append(rng.randrange(256))
        pixels.extend(b'\xA5' * (stride - width * 3))
    header = bytearray(54)
    struct.pack_into('<2sIHHI', header, 0, b'BM', 54 + len(pixels), 0, 0, 54)
    struct.pack_into('<IiiHHIIiiII', header, 14, 40, width, signed_height,
                     1, 24, 0, len(pixels), 0, 0, 0, 0)
    with open(path, 'wb') as f:
        f.write(header)
        f.write(pixels)


def check_output(program, folder, width, height, seed=None):
    src = os.path.join(folder, 'input.bmp')
    sequential = os.path.join(folder, 'sequential.bmp')
    planar = os.path.join(folder, 'planar.bmp')
    if seed is None:
        create_bmp(src, width, height)
    else:
        create_random_bmp(src, width, height, seed)
    subprocess.run([program, src, sequential], check=True, capture_output=True)
    subprocess.run([program, '--planar', src, planar], check=True, capture_output=True)
    with open(sequential, 'rb') as f:
        reference = f.read()
    with open(planar, 'rb') as f:
        actual = f.read()
    assert reference == actual, f'Output mismatch: width={width}, height={height}, seed={seed}'


def main():
    if len(sys.argv) != 2:
        print('Usage: python tests/test_planar.py PATH_TO_PROGRAM')
        return 1
    program = os.path.abspath(sys.argv[1])
    count = 0
    with tempfile.TemporaryDirectory() as folder:
        for width in (1, 7, 8, 9, 13, 17, 640):
            for height in (3, -3):
                check_output(program, folder, width, height)
                count += 1
        for i, width in enumerate((1, 2, 7, 8, 9, 13, 17, 31, 32, 33, 65, 127, 128, 129, 257)):
            for height in (7, -7):
                check_output(program, folder, width, height, seed=i + 123)
                count += 1
    print(f'PASS: {count} cases; planar-float C output matches sequential C byte-for-byte.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
