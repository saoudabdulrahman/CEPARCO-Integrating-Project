"""Generate deterministic, uncompressed 24-bit BMP images for benchmarking.
No third-party dependencies. Not part of any timed conversion region.
"""
import os
import random
import struct
import sys

SIZES = ((640, 480), (1280, 720), (1920, 1080), (3840, 2160), (641, 479))


def create_bmp(path, width, height):
    stride = (width * 3 + 3) // 4 * 4
    padding = b'\xA5' * (stride - width * 3)
    pixel_bytes = stride * height
    header = bytearray(54)
    struct.pack_into('<2sIHHI', header, 0, b'BM', 54 + pixel_bytes, 0, 0, 54)
    struct.pack_into('<IiiHHIIiiII', header, 14, 40, width, height, 1, 24, 0,
                     pixel_bytes, 0, 0, 0, 0)
    random_bytes = random.Random(10000 + width + height)
    with open(path, 'wb') as f:
        f.write(header)
        for _ in range(height):
            f.write(random_bytes.randbytes(width * 3))
            f.write(padding)


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else 'benchmark_inputs'
    os.makedirs(folder, exist_ok=True)
    for w, h in SIZES:
        path = os.path.join(folder, f'{w}x{h}.bmp')
        if not os.path.exists(path):
            create_bmp(path, w, h)
        print(path)


if __name__ == '__main__':
    main()
