import subprocess
import sys

def main():
    cols = int(sys.argv[1])
    rows = int(sys.argv[2])
    time_seq = float(subprocess.check_output(['./run.sh', str(cols), str(rows)]))
    # x = subprocess.check_output(['ls'])
    res_mpi_str = subprocess.check_output(['./run_mpi.sh', str(cols), str(rows)])
    res_mpi = [s.strip().split(' ') for s in res_mpi_str.splitlines()]
    out = [(int(t[0]), time_seq/float(t[1])) for t in res_mpi]
    filename_prefix = 'perf%iX%i' % (cols, rows)
    perf_filename = filename_prefix + '.dat'
    plot_filename = filename_prefix + '.png'
    with open(perf_filename, 'w') as f:
        for r in out:
            f.write('%i %.2lf\n' % (r[0], r[1]))
    subprocess.check_output(['gnuplot', '-e', "in_filename='%s'" % perf_filename, '-e', "out_filename='%s'" % plot_filename, 'perf.gnu'])
    return 0

if __name__ == "__main__":
    main()