CC=gcc
CFLAGS=-O3 -Wall -Wextra -Werror -pg

all: p1.o
	$(CC) $(CFLAGS) p1.o -o p1
p1.o: p1.c
	$(CC) $(CFLAGS) -c p1.c
	
clean:
	rm -f p1.o p1 gmon.out

