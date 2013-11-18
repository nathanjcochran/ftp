CC=gcc
DEBUG=-g
CFLAGS=$(DEBUG) -Wall -Wshadow -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement
PROGS=ftserve ftclient

all: $(PROGS)

server: ftserve

client: ftclient

ftserve: ftserve.o ftutil.o
	$(CC) $(CFLAGS) -o $@ ftserve.o ftutil.o

ftclient: ftclient.o ftutil.o
	$(CC) $(CFLAGS) -o $@ ftclient.o ftutil.o
    
ftserve.o: ftserve.c ftserve.h
	$(CC) $(CFLAGS) -c ftserve.c

ftclient.o: ftclient.c ftclient.h
	$(CC) $(CFLAGS) -c ftclient.c

ftutil.o: ftutil.c ftutil.h
	$(CC) $(CFLAGS) -c ftutil.c

clean:
	rm -f $(PROGS) *.o *~

