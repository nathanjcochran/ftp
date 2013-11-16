CC=gcc
DEBUG=-g
CFLAGS=$(DEBUG) -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement
PROGS=ftserve ftclient

all: $(PROGS)

server: ftserve

client: ftclient

ftserve: ftserve.o ftutil.o
	$(CC) $(CFLAGS) -o $@ ftserve.o ftutil.o

ftclient: ftclient.o ftutil.o
	$(CC) $(CFLAGS) -o $@ ftclient.o ftutil.o
    
ftserve.o: ftserve.c ftutil.c
	$(CC) $(CFLAGS) -c $^

ftclient.o: ftclient.c 
	$(CC) $(CFLAGS) -c $^

ftutil.o: ftutil.c ftutil.h
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(PROGS) *.o *~

