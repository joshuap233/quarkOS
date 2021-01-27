SRC          = $(shell pwd)/../src
INCLUDE      = $(SRC)/include
CC           = gcc
CYTHON       = cython
PYTHON       = python3

LIB_SETUP    = setup.py
LIB 	     = build/tool

CYTHON_FLAGS = -3  --embed
CLIB_FLAGS   = -std=gnu11 -ffreestanding -O0 -Wall -Wextra -g -I $(INCLUDE)
CTEST_FLAGS  = -I $(INCLUDE) -I /usr/include/python3.8 -L$(LIB) -lpython3.8

TEST_SOURCES := $(notdir $(shell find . -type f -name "test_*.pyx"))
TEST_TARGET  := $(addprefix build/,$(patsubst %.pyx,%,$(TEST_SOURCES)))

LIB_SOURCES  := $(addprefix $(SRC)/, lib/qmath.c lib/qstring.c)

LIB_OBJECTS  := $(addprefix build/,$(notdir $(patsubst %.c,%.o,$(LIB_SOURCES))))

VPATH = ./build:../src/lib/

.PHONY: clean test

all: $(TEST_TARGET)

$(TEST_TARGET): build/%: %.pyx $(LIB_OBJECTS) build/*.so
	$(CYTHON) $(CYTHON_FLAGS) $< -o $@.c
	$(CC) build/*.so $(LIB_OBJECTS) $(patsubst %.pyx,build/%.c,$<) $(CTEST_FLAGS) -o $@

$(LIB_OBJECTS): build/%.o: %.c
	$(CC) $(CLIB_FLAGS) -c $(filter %$<,$(LIB_SOURCES)) -o $@
#

build/*.so: tool.pyx $(LIB_SETUP)
	$(PYTHON) $(LIB_SETUP) build_ext --inplace
	mv *.so build/

test: $(TEST_TARGET)
	@for i in $(TEST_TARGET); \
	do \
	  $${i} || exit 1;\
  	done; \



clean:
	rm -rf build/*
