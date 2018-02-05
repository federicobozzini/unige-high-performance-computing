#!/bin/bash

ROWS=${1:-1111}
COLS=${2:-2222}

gcc mandelbrot_seq.c -fopenmp -O3 -o mandelbrot_seq && ./mandelbrot_seq $ROWS $COLS