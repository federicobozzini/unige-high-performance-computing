#!/bin/bash

ROWS=${1:-1111}
COLS=${2:-2222}
TASK_SIZE=${3:-333}
NODES=${4:-17}
X0=${5:--2.5}
Y0=${6:--1}
DX=${7:-3.5}
DY=${8:-2}

mpicc mandelbrot_mpi.c -fopenmp -lm -O3 -o mandelbrot_mpi && mpiexec -machinefile macchine -n $NODES ./mandelbrot_mpi $ROWS $COLS $TASK_SIZE $X0 $Y0 $DX $DY
