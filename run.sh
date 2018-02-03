#!/bin/bash

gcc mandelbrot.c -O3 -o mandelbrot && ./mandelbrot $1 $2