#!/bin/bash

rm mandelbrot_seq.dat
rm mandelbrot_omp.dat
ROWS=${1:-111}
COLS=${1:-222}
./run_seq.sh $ROWS $COLS > /dev/null
./run_omp.sh $ROWS $COLS > /dev/null
if ! cmp mandelbrot_seq.dat mandelbrot_omp.dat >/dev/null 2>&1
then
  >&2 echo 'different'
  exit 1
fi