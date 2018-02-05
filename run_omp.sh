#!/bin/bash

ROWS=${1:-1111}
COLS=${2:-2222}

gcc mandelbrot_omp.c -fopenmp -O3 -o mandelbrot_omp && ./mandelbrot_omp $ROWS $COLS