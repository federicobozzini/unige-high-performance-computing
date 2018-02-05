set term png
set out out_filename
set size ratio 1
set xtics rotate by 45 right
set xlabel "# worker nodes"
set ylabel "speedup"
plot in_filename using 1:2:xtic(1) notitle with lines
set out