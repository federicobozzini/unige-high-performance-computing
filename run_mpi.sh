#!/bin/bash

ROWS=${1:-1111}
COLS=${2:-2222}
NODES=${3:-17}

mpicc mandelbrot_mpi.c -fopenmp -lm -O3 -o mandelbrot_mpi && mpiexec -machinefile macchine -n $NODES ./mandelbrot_mpi $ROWS $COLS
