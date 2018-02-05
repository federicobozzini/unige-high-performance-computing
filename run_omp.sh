#!/bin/bash

gcc mandelbrot_omp.c -fopenmp -O3 -o mandelbrot_omp && ./mandelbrot_omp $1 $2