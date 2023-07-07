OUTPUTNAME=renderer
CC=gcc
CFLAGS=-g -D DEBUG
LDFLAGS=-lglfw -lvulkan -lcglm -lm
SRC=$(shell find src -type f -name "*.c")
OBJ=$(patsubst src/%.c, obj/%.o, $(SRC))

.PHONY: all run clean

all: $(OUTPUTNAME) hello.vert.spv hello.frag.spv

$(OUTPUTNAME): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

obj/%.o: src/%.c src/lime.h
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

hello.vert.spv: src/shaders/hello.vert
	glslc $< -o $@

hello.frag.spv: src/shaders/hello.frag
	glslc $< -o $@

run: all
	./$(OUTPUTNAME)
clean:
	rm -fr obj $(OUTPUTNAME) hello.vert.spv hello.frag.spv
