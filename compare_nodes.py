import subprocess
import sys

def main():
    cols = int(sys.argv[1])
    rows = int(sys.argv[2])
    nodes_max = 25;
    time_seq = float(subprocess.check_output(['./run.sh', str(cols), str(rows)]))
    out = []
    for n in range(2, nodes_max):
        workers = n-1
        res_mpi_str = subprocess.check_output(['./run_mpi.sh', str(cols), str(rows), str(n)])
        res_mpi = [s.strip().split(' ') for s in res_mpi_str.splitlines()]
        best_speedup = max([time_seq/float(t[1]) for t in res_mpi])
        out.append((workers, best_speedup))
    filename_prefix = 'compare%iX%i' % (cols, rows)
    perf_filename = filename_prefix + '.dat'
    plot_filename = filename_prefix + '.png'
    with open(perf_filename, 'w') as f:
        for r in out:
            f.write('%i %.2lf\n' % (r[0], r[1]))
    subprocess.check_output(['gnuplot', '-e', "in_filename='%s'" % perf_filename, '-e', "out_filename='%s'" % plot_filename, 'compare.gnu'])
    return 0

if __name__ == "__main__":
    main()