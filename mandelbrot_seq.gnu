set term png
set out "images/mandelbrot_seq.png"
set pm3d map
set size ratio 1
set pm3d interpolate 0,0
set xtics format " " 
set ytics format " " 
stats 'results/mandelbrot_seq.dat' nooutput
N = STATS_records
M = STATS_columns
splot 'results/mandelbrot_seq.dat' skip 1 matrix using ($1/(N-1)*3.5-2.5):($2/(M-1)*2-1):3 notitle
set out