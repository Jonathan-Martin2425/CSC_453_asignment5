CC = cc

FLAGS = -g -Wall

all: minls minget

minget: minget.o util.o partition.o
	$(CC) -o minget minget.o partition.o util.o

minls: minls.o util.o partition.o
	$(CC) -o minls minls.o partition.o util.o

minls.o: minls.c
	$(CC) $(FLAGS) -c minls.c

minget.o: minget.c
	$(CC) $(FLAGS) -c minget.c

util.o: util.c
	$(CC) $(FLAGS) -c util.c

partition.o: partition.c
	$(CC) $(FLAGS) -c partition.c

clean:
	rm *.o minls minget
