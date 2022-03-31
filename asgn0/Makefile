CFLAGS=-Wall -Wextra -Werror -pedantic
CC=clang $(CFLAGS)

split:		split.o
			$(CC) -o split split.o
split.o:	split.c
			$(CC) -c split.c
clean:
			rm -f split split.o
infer:
			make clean; infer-capture -- make; infer-analyze -- make
all:		split