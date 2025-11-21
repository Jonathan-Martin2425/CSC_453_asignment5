CC = cc

minls: minls.o
	$(CC) -o minls minls.o

minls.o: minls.c
	$(CC) -c minls.c