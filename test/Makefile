OBJS = qstring.o
SCRS = qstring.c

INCLUDE    = ../src/include
CC         = gcc
CLIB_FLAGS = -std=gnu11 -ffreestanding -O2 -Wall -Wextra -g -I $(INCLUDE)
C_FLAGS    = -std=gnu11 -O2 -Wall -Wextra -g -I $(INCLUDE)

TEST_SOURCES = $(shell find . -type f -name "*_test.c")
TEST_OBJECTS := $(addprefix build/,$(patsubst %.c,%,$(TEST_SOURCES)))

# 添加头文件为依赖
LIB_SOURCES := $(shell find ../src/lib -type f -name "*.c")
LIB_OBJECTS := $(addprefix build/,$(notdir $(patsubst %.c,%.o,$(LIB_SOURCES))))

VPATH = ./build:../src/lib/
vpath %.c

.PHONY: clean test

all: $(TEST_OBJECTS)

$(TEST_OBJECTS): build/%: %.c tool.o $(LIB_OBJECTS)
	$(CC) $(C_FLAGS) $(LIB_OBJECTS) ./build/tool.o $< -o $@

# TODO: 依赖替换为简单的 %.c,然后使用 vapth 添加搜索路径
# $(LIB_OBJECTS): %.o: $(filter $(notdir %.c),$(LIB_SOURCES))
	# $(CC) $(C_FLAGS) -c $(filter %$(patsubst build/%.o,%.c,$@),$(LIB_SOURCES)) -o $@

$(LIB_OBJECTS): build/%.o: %.c
	$(CC) $(C_FLAGS) -c $(filter %$<,$(LIB_SOURCES)) -o $@

tool.o: tool.h tool.c
	$(CC) $(C_FLAGS) tool.c -c -o build/tool.o

test: $(TEST_OBJECTS)
	./$<

clean:
	-rm build/* 
