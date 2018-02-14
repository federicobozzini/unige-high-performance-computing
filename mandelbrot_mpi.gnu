set term png
set out "images/mandelbrot_mpi.png"
set pm3d map
set size ratio 1
set pm3d interpolate 0,0
set xtics format " " 
set ytics format " " 
stats 'results/mandelbrot_mpi.dat' nooutput
N = STATS_records
M = STATS_columns
splot 'results/mandelbrot_mpi.dat' skip 1 matrix using ($1/(N-1)*3.5-2.5):($2/(M-1)*2-1):3 notitle
set out