#!/bin/bash

ROWS=${1:-1111}
COLS=${2:-2222}
X0=${3:--2.5}
Y0=${4:--1}
DX=${5:-3.5}
DY=${6:-2}

gcc mandelbrot_omp.c -fopenmp -O3 -o mandelbrot_omp && ./mandelbrot_omp $ROWS $COLS $X0 $Y0 $DX $DY