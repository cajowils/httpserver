CFLAGS=-Wall -Wextra -Werror -pedantic
CC=clang $(CFLAGS)

httpserver:		httpserver.o methods.o requests.o
				$(CC) -o httpserver httpserver.o methods.o requests.o
httpserver.o:	methods.h requests.h httpserver.c
				$(CC) -c httpserver.c
methods.o:		methods.h methods.c
				$(CC) -c methods.c
requests.o:		requests.h requests.c
				$(CC) -c requests.c
clean:
				rm -f httpserver *.o
infer:
				make clean; infer-capture -- make; infer-analyze -- make
all:			httpserver