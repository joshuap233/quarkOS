OBJS = qstring.o
SCRS = qstring.c

INCLUDE = ../include
CC = gcc
CLIB_FLAGS = -std=gnu11 -ffreestanding -O2 -Wall -Wextra -I $(INCLUDE)
CFLAGS = -std=gnu11 -O2 -Wall -Wextra -I $(INCLUDE)

vpath %.h ../include
vpath %.c ../lib

.PHONY: clean test

lib_test: qstring.o lib_test.c
	$(CC) $(CFLAGS) qstring.o lib_test.c -o lib_test

qstring.o : qstring.c qstring.h
	$(CC) $(CLIB_FLAGS) ../lib/qstring.c -c -o qstring.o


clean:
	-rm *.o *_test
