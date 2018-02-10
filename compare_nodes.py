#!/usr/bin/env python

import subprocess
import sys


def main():
    numargs = len(sys.argv) - 1
    cols = int(sys.argv[1]) if numargs > 0 else 1000
    rows = int(sys.argv[2]) if numargs > 1 else cols
    size = cols * rows
    task_size_min = int(sys.argv[3]) if numargs > 2 else size / 1000
    task_size_max = int(sys.argv[4]) if numargs > 3 else size / 50
    task_size_step = int(sys.argv[5]) if numargs > 4 else size / 100
    nodes_max = 20
    time_seq = float(subprocess.check_output(
        ['./run.sh', str(cols), str(rows)]))
    print "seq time=%.2lf" % time_seq
    task_sizes = range(task_size_min, task_size_max + 1, task_size_step)
    out = []
    for n in range(2, nodes_max):
        best_time_mpi = sys.float_info.max
        for task_size in task_sizes:
            workers = n - 1
            time_mpi = float(subprocess.check_output(
                ['./run_mpi.sh', str(cols), str(rows), str(task_size), str(n)]))
            best_time_mpi = min(best_time_mpi, time_mpi)
        best_speedup = time_seq / best_time_mpi
        print "mpi workers=%i time=%.2lf speedup=%.2lf" % (workers, best_time_mpi, best_speedup)
        out.append((workers, best_speedup))
    filename_prefix = 'compare%iX%i' % (cols, rows)
    perf_filename = filename_prefix + '.dat'
    plot_filename = filename_prefix + '.png'
    with open(perf_filename, 'w') as f:
        for r in out:
            f.write('%i %.2lf\n' % (r[0], r[1]))
    subprocess.check_output(['gnuplot', '-e', "in_filename='%s'" %
                             perf_filename, '-e', "out_filename='%s'" % plot_filename, 'compare.gnu'])
    return 0


if __name__ == "__main__":
    main()
