CC=gcc
CFLAGS=-O3 -Wall -Wextra -Werror -pg -g

all: life.o tools.o
	$(CC) $(CFLAGS) life.o tools.o -o life
life.o: life.c tools/tools.h
	$(CC) $(CFLAGS) -c life.c
tools.o: tools/tools.c tools/tools.h
	$(CC) $(CFLAGS) -c tools/tools.c
clean:
	rm -f life.o tools.o life gmon.out

