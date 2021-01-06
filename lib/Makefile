OBJS = qstring.o
SCRS = qstring.c

INCLUDE = ../include
CC = i686-elf-gcc
CFLAGS = -std=gnu11 -ffreestanding -O2 -Wall -Wextra -I $(INCLUDE)

vpath %.h ../include

.PHONY: clean test

target: qstring.o

qstring.o : qstring.c qstring.h
	$(CC) $(CFLAGS) qstring.c -c -o qstring.o


clean:
	-rm *.o
