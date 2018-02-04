#!/bin/bash

mpicc mandelbrot_mpi.c -fopenmp -O3 -o mandelbrot_mpi && mpiexec -machinefile macchine -n 6 ./mandelbrot_mpi $1 $2
