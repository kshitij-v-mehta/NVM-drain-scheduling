all: drainer

drainer: logger.o main.o monitor.o copier.o shm.o utils.o file_utils.o mpi_utils.o
	mpicc -fopenmp main.o utils.o file_utils.o mpi_utils.o logger.o  monitor.o copier.o shm.o -o drainer

monitor.o : monitor.c shm.o
	mpicc -c -g -fopenmp monitor.c

shm.o : shm.c
	mpicc -c -g -fopenmp shm.c

copier.o : copier.c
	mpicc -c -g -fopenmp copier.c

main.o : monitor.o utils.o copier.o shm.o
	mpicc -c -g -fopenmp main.c

utils.o : utils.c
	mpicc -c -g -fopenmp utils.c

file_utils.o : file_utils.c
	mpicc -c -g -fopenmp file_utils.c

mpi_utils.o : mpi_utils.c
	mpicc -c -g -fopenmp mpi_utils.c

logger.o : logger.c
	mpicc -c -g -fopenmp logger.c


.PHONY: all clean run
clean:
	rm -f *.o drainer

run:
	mpirun -np $(n) ./drainer -n 2 -t 1024 -p 0 -s 0 -f equilibrium.bp -w 5

