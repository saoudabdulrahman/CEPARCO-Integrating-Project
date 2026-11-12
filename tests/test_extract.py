"""Check packed-BGR extraction and the protected-memory boundary."""
import os
import subprocess
import sys
import tempfile
from test_grayscale import create_bmp
from test_planar import create_random_bmp


def run(program, filename):
    process = subprocess.run([program, '--extract-check', filename],
                             capture_output=True, text=True)
    if process.returncode != 0 or 'PASS: packed BGR extraction matches C' not in process.stdout:
        raise AssertionError(f'{filename}: exit={process.returncode}\n{process.stdout}\n{process.stderr}')
    return process.stdout


def main():
    if len(sys.argv) != 2:
        print('Usage: python tests\\test_extract.py grayscale.exe')
        return 1
    exe = os.path.abspath(sys.argv[1])
    cases = 0
    with tempfile.TemporaryDirectory() as temp:
        path = os.path.join(temp, 'input.bmp')
        for w in (1, 2, 7, 8, 9, 13, 16, 17, 31, 32, 33, 640, 641):
            for h in (1, -1, 3, -3):
                create_bmp(path, w, h)
                result = run(exe, path)
                if w >= 8 and 'SIMD blocks=0 ' in result:
                    raise AssertionError('No vector blocks for width ' + str(w))
                cases += 1
        for i, w in enumerate((8, 9, 13, 17, 23, 31, 32, 33, 65, 127, 128, 129, 257)):
            for h in (5, -5):
                create_random_bmp(path, w, h, seed=4100 + i)
                run(exe, path)
                cases += 1

    guard = subprocess.run([exe, '--extract-guard'], capture_output=True, text=True)
    if guard.returncode != 0 or 'PASS: packed 24-byte load at a guarded page boundary' not in guard.stdout:
        raise AssertionError('Guard page test failed:\n' + guard.stdout + '\n' + guard.stderr)
    print(f'PASS: {cases} BMP extraction cases plus guarded boundary load.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
