all: drainer

drainer: main.o monitor.o copier.o shm.o utils.o
	mpicc -fopenmp main.o utils.o monitor.o copier.o shm.o -o drainer

monitor.o : monitor.c shm.o
	mpicc -c monitor.c

shm.o : shm.c
	mpicc -c shm.c

copier.o : copier.c
	mpicc -c copier.c

main.o : monitor.o utils.o copier.o shm.o
	mpicc -c main.c

utils.o : utils.c
	mpicc -c utils.c

.PHONY: all clean
clean:
	rm -f *.o drainer

