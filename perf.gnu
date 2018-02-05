set term png
set out out_filename
set size ratio 1
set logscale x 20
set xtics rotate by 45 right
plot in_filename using 1:2 title 'Removed' with lines
set out