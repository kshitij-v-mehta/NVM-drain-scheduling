all: drainer

drainer: main.o monitor.o copier.o shm.o utils.o file_utils.o mpi_utils.o
	mpicc -fopenmp main.o utils.o file_utils.o mpi_utils.o monitor.o copier.o shm.o -o drainer

monitor.o : monitor.c shm.o
	mpicc -c monitor.c

shm.o : shm.c
	mpicc -c shm.c

copier.o : copier.c
	mpicc -c copier.c

main.o : monitor.o utils.o copier.o shm.o
	mpicc -c -fopenmp main.c

utils.o : utils.c
	mpicc -c utils.c

file_utils.o : file_utils.c
	mpicc -c file_utils.c

mpi_utils.o : mpi_utils.c
	mpicc -c mpi_utils.c


.PHONY: all clean run
clean:
	rm -f *.o drainer

run:
	mpirun -np 2 ./drainer -n 2 -t 1024 -p 0 -s 0 -f equilibrium.bp -w 2

