CC = cc

min: minls minget

minget: minget.o util.o partition.o
	$(CC) -o minget minget.o partition.o util.o

minls: minls.o util.o partition.o
	$(CC) -o minls minls.o partition.o util.o

minls.o: minls.c
	$(CC) -c minls.c

minget.o: minget.c
	$(CC) -c minget.c

util.o: util.c
	$(CC) -c util.c

partition.o: partition.c
	$(CC) -c partition.c

clean:
	rm *.o minls
