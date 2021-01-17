INCLUDE    = ../src/include
CC         = gcc
CLIB_FLAGS = -std=gnu11 -ffreestanding -O0 -Wall -Wextra -g -I $(INCLUDE)
C_FLAGS    = -std=gnu11 -O0 -Wall -Wextra -g -I $(INCLUDE)

TEST_SOURCES := $(notdir $(shell find . -type f -name "*_test.c"))
TEST_TARGET  := $(addprefix build/,$(patsubst %.c,%,$(TEST_SOURCES)))

LIB_SOURCES  := $(shell find ../src/lib -type f -name "*.c")
LIB_OBJECTS  := $(addprefix build/,$(notdir $(patsubst %.c,%.o,$(LIB_SOURCES))))

VPATH = ./build:../src/lib/

.PHONY: clean test

all: $(TEST_TARGET)

$(TEST_TARGET): build/%: %.c $(LIB_OBJECTS) tool.o
	$(CC) $(C_FLAGS) ./build/tool.o $(LIB_OBJECTS) $< -o $@

$(LIB_OBJECTS): build/%.o: %.c
	$(CC) $(C_FLAGS) -c $(filter %$<,$(LIB_SOURCES)) -o $@

tool.o: tool.h tool.c
	$(CC) $(C_FLAGS) tool.c -c -o build/tool.o

test: $(TEST_TARGET)
	./$<

clean:
	rm build/*
