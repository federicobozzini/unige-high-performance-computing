#!/bin/bash

rm results/mandelbrot_seq.dat
rm results/mandelbrot_mpi.dat
ROWS=${1:-1111}
COLS=${2:-2222}
./run_seq.sh $ROWS $COLS > /dev/null
./run_mpi.sh $ROWS $COLS > /dev/null
if ! cmp results/mandelbrot_seq.dat results/mandelbrot_mpi.dat >/dev/null 2>&1
then
  >&2 echo 'different'
  exit 1
fi