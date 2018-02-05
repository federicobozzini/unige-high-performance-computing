#!/bin/bash

gcc mandelbrot_seq.c -fopenmp -O3 -o mandelbrot_seq && ./mandelbrot_seq $1 $2