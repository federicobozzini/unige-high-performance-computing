set term png
set out out_filename
set size ratio 1
set logscale x 20
set xtics rotate by 45 right
set xlabel "task size"
set ylabel "speedup"
plot in_filename using 1:2 notitle with lines
set out