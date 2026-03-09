CC     = gcc
CFLAGS = -O2 -Wall
LIBS   = -lm

all: serial omp_edge

serial: serial.c image_io.h
	$(CC) $(CFLAGS) -o serial serial.c $(LIBS)

omp_edge: omp_edge.c image_io.h
	$(CC) $(CFLAGS) -fopenmp -o omp_edge omp_edge.c $(LIBS)

clean:
	rm -f serial omp_edge output_*.pgm