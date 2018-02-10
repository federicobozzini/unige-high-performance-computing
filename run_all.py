#!/usr/bin/env python

import subprocess
import sys

def main():
    numargs = len(sys.argv) - 1
    cols = int(sys.argv[1]) if numargs > 0 else 1000
    rows = int(sys.argv[2]) if numargs > 1 else cols
    size = cols * rows
    task_size_min = int(sys.argv[3]) if numargs > 2 else size/500
    task_size_max = int(sys.argv[4]) if numargs > 3 else size/50
    task_size_step = int(sys.argv[5]) if numargs > 4 else size/500
    task_sizes = range(task_size_min, task_size_max+1, task_size_step)
    if len(task_sizes) == 0:
        print 'No task sizes'
        return 1
    time_seq = float(subprocess.check_output(['./run.sh', str(cols), str(rows)]))
    print "seq time=%.2lf" % time_seq
    time_mpi_list = []
    for task_size in task_sizes:
        time_mpi = float(subprocess.check_output(['./run_mpi.sh', str(cols), str(rows), str(task_size)]))
        print "mpi task size=%i time=%.2lf" % (task_size, time_mpi)
        time_mpi_list.append((task_size, time_mpi))
    out = [(t[0], time_seq/t[1]) for t in time_mpi_list]
    filename_prefix = 'perf%iX%i' % (cols, rows)
    perf_filename = 'results/' + filename_prefix + '.dat'
    plot_filename = 'images/' + filename_prefix + '.png'
    with open(perf_filename, 'w') as f:
        for r in out:
            f.write('%i %.2lf\n' % (r[0], r[1]))
    subprocess.check_output(['gnuplot', '-e', "in_filename='%s'" % perf_filename, '-e', "out_filename='%s'" % plot_filename, 'perf.gnu'])
    return 0

if __name__ == "__main__":
    main()