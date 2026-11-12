"""Run a program on generated BMPs; write measurement summaries to CSV."""
import csv
import os
import subprocess
import sys
from create_bench_inputs import create_bmp, SIZES


def main():
    if len(sys.argv) < 2 or len(sys.argv) > 4:
        print('Usage: python tests\\collect_benchmarks.py grayscale.exe [runs] [output.csv]')
        return 1
    program = os.path.abspath(sys.argv[1])
    runs = int(sys.argv[2]) if len(sys.argv) >= 3 else 30
    csv_file = sys.argv[3] if len(sys.argv) >= 4 else 'benchmark_results.csv'
    if not 30 <= runs <= 200:
        print('Runs must be between 30 and 200.')
        return 1
    folder = 'benchmark_inputs'
    os.makedirs(folder, exist_ok=True)
    rows = []
    raw_rows = []
    for w, h in SIZES:
        filename = os.path.join(folder, f'{w}x{h}.bmp')
        if not os.path.isfile(filename):
            create_bmp(filename, w, h)
        print(f'Measuring {w} x {h} ...', flush=True)
        result = subprocess.run([program, '--bench', os.path.abspath(filename), str(runs)],
                                check=True, text=True, capture_output=True)
        if 'PASS: all full-output' not in result.stdout:
            raise RuntimeError('Correctness status missing: ' + result.stdout)
        for line in result.stdout.splitlines():
            if line.startswith('CSV_FULL,') or line.startswith('CSV_KERNEL,'):
                parts = line.split(',')
                if len(parts) == 5 and parts[1] != 'mode':
                    category, mode, mean, median, speedup = parts
                    rows.append({'width': w, 'height': h, 'pixels': w * h,
                                 'comparison': 'complete_in_memory' if category == 'CSV_FULL' else 'kernel_only_prepared',
                                 'implementation': mode, 'mean_ms': mean,
                                 'median_ms': median, 'speedup': speedup,
                                 'runs': runs, 'warmups': 5})
            if line.startswith('CSV_SAMPLE,'):
                parts = line.split(',')
                if len(parts) == 5 and parts[1] != 'comparison':
                    _, comparison, mode, iteration, time_ms = parts
                    raw_rows.append({'width': w, 'height': h, 'pixels': w * h,
                                     'comparison': comparison, 'implementation': mode,
                                     'iteration': iteration, 'time_ms': time_ms})
    with open(csv_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['width', 'height', 'pixels', 'comparison',
                                              'implementation', 'mean_ms', 'median_ms',
                                              'speedup', 'runs', 'warmups'])
        writer.writeheader()
        writer.writerows(rows)
    name, ext = os.path.splitext(csv_file)
    raw_csv = name + '_samples' + (ext or '.csv')
    with open(raw_csv, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['width', 'height', 'pixels', 'comparison',
                                              'implementation', 'iteration', 'time_ms'])
        writer.writeheader()
        writer.writerows(raw_rows)
    print(f'Saved {len(rows)} summary rows to {csv_file}')
    print(f'Saved {len(raw_rows)} individual timings to {raw_csv}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
