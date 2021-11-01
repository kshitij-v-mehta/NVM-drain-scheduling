#!/bin/bash

rm -rf ./mnt/bb/kmehta/globalArray.bp/*
rm -f globalArray.bp/*
rm -f drainer.out

mpirun -np 1 ./adios-writer &
DRAINER_LOG_LEVEL=INFO OMP_NUM_THREADS=1 mpirun -np 1 ./drainer -n 1 -t $((64*10)) -p 0 -s 0 -f globalArray.bp -w 1 -m mnt/bb/kmehta -d 2 -i 5000 -a 1

wait

