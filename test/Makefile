OBJS = qstring.o
SCRS = qstring.c

INCLUDE = ../include
CC = gcc
CLIB_FLAGS = -std=gnu11 -ffreestanding -O2 -Wall -Wextra -g -I $(INCLUDE)
CFLAGS = -std=gnu11 -O2 -Wall -Wextra -g -I $(INCLUDE)

vpath %.h ../include
vpath %.c ../lib

.PHONY: clean test

libqstring_test: qstring.o libqstring_test.c tool.o
	$(CC) $(CFLAGS) tool.o qstring.o libqstring_test.c -o libqstring_test

qstring.o : qstring.c qstring.h
	$(CC) $(CLIB_FLAGS) ../lib/qstring.c -c -o qstring.o

tool.o: tool.h tool.c
	$(CC) $(CFLAGS) tool.c -c -o tool.o

clean:
	-rm *.o *_test
