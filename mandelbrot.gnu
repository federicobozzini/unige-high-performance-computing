set term png
set out "mandelbrot.png"
set pm3d map
set size ratio 1
set pm3d interpolate 0,0
splot 'mandelbrot.dat' matrix
set out

