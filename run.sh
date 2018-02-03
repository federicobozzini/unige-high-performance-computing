#!/bin/bash

gcc mandelbrot.c -fopenmp -O3 -o mandelbrot && ./mandelbrot $1 $2