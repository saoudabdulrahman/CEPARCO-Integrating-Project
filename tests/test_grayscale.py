"""Minimal tests for the sequential BMP converter; standard-library only."""
import os
import struct
import subprocess
import sys
import tempfile

COLORS = [(0, 0, 255), (0, 255, 0), (255, 0, 0),
          (0, 0, 0), (255, 255, 255), (128, 128, 128)]  # B,G,R
EXPECTED = [76, 150, 29, 0, 255, 128]


def create_bmp(path, width, signed_height):
    height = abs(signed_height)
    stride = ((width * 3 + 3) // 4) * 4
    pixels = bytearray()
    for row in range(height):
        for col in range(width):
            pixels.extend(COLORS[(row * width + col) % len(COLORS)])
        pixels.extend(b'\xA5' * (stride - width * 3))
    header = bytearray(54)
    struct.pack_into('<2sIHHI', header, 0, b'BM', 54 + len(pixels), 0, 0, 54)
    struct.pack_into('<IiiHHIIiiII', header, 14, 40, width, signed_height,
                     1, 24, 0, len(pixels), 0, 0, 0, 0)
    with open(path, 'wb') as f:
        f.write(header)
        f.write(pixels)
    return bytes(header + pixels)


def run_test(program, width, signed_height, folder):
    src = os.path.join(folder, 'source.bmp')
    dst = os.path.join(folder, 'result.bmp')
    original = create_bmp(src, width, signed_height)
    subprocess.run([program, src, dst], check=True, capture_output=True)
    result = open(dst, 'rb').read()
    assert len(result) == len(original)
    assert result[:54] == original[:54]  # BMP header preserved
    stride = ((width * 3 + 3) // 4) * 4
    for row in range(abs(signed_height)):
        base = 54 + row * stride
        for col in range(width):
            p = base + col * 3
            expected = EXPECTED[(row * width + col) % len(EXPECTED)]
            assert result[p:p + 3] == bytes([expected] * 3), (width, signed_height, row, col)
        assert result[base + width * 3:base + stride] == original[base + width * 3:base + stride]


def main():
    if len(sys.argv) != 2:
        print('Usage: python tests/test_grayscale.py PATH_TO_PROGRAM')
        return 1
    program = os.path.abspath(sys.argv[1])
    with tempfile.TemporaryDirectory() as folder:
        for width in [1, 7, 8, 9, 13, 17, 640]:
            for height in [3, -3]:
                run_test(program, width, height, folder)
    print('PASS: 14 BMP test cases; padding, orientation, and pixel colors verified.')
    return 0

if __name__ == '__main__':
    sys.exit(main())
