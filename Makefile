all: drainer

drainer: logger.o main.o monitor.o copier.o shm.o utils.o file_utils.o mpi_utils.o
	mpicc -g ${CFLAGS} -fopenmp main.o utils.o file_utils.o mpi_utils.o logger.o  monitor.o copier.o shm.o draining.o -o drainer

monitor.o : monitor.c shm.o
	mpicc -c -g ${CFLAGS} -fopenmp monitor.c

shm.o : shm.c
	mpicc -c -g ${CFLAGS} -fopenmp shm.c

copier.o : copier.c
	mpicc -c -g ${CFLAGS} -fopenmp copier.c

draining.o : draining.c
	mpicc -c -g ${CFLAGS} draining.c

main.o : main.c monitor.o utils.o copier.o shm.o draining.o
	mpicc -c -g ${CFLAGS} -fopenmp main.c

utils.o : utils.c
	mpicc -c -g ${CFLAGS} -fopenmp utils.c

file_utils.o : file_utils.c
	mpicc -c -g ${CFLAGS} -fopenmp file_utils.c

mpi_utils.o : mpi_utils.c
	mpicc -c -g ${CFLAGS} -fopenmp mpi_utils.c

logger.o : logger.c
	mpicc -c -g ${CFLAGS} -fopenmp logger.c


.PHONY: all clean run
clean:
	rm -f *.o drainer

run:
	rm -rf gs.bp/*
	DRAINER_LOG_LEVEL=INFO OMP_NUM_THREADS=3 mpirun -np $(n) ./drainer -n 2 -t 65536 -p 0 -s 0 -f gs.bp -m mnt/bb/kmehta -w 5 -d 1
	./verify.sh
