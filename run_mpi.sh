#!/bin/bash

ROWS=${1:-111}
COLS=${2:-222}
NODES=${3:-15}

mpicc mandelbrot_mpi.c -fopenmp -lm -O3 -o mandelbrot_mpi && mpiexec -machinefile macchine -n $NODES ./mandelbrot_mpi $ROWS $COLS
