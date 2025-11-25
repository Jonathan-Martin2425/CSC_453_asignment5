CC = cc

minls: minls.o util.o partition.o
	$(CC) -o minls minls.o partition.o util.o

minls.o: minls.c
	$(CC) -c minls.c

util.o: util.c
	$(CC) -c util.c

partition.o: partition.c
	$(CC) -c partition.c
