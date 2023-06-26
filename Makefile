OUTPUTNAME=renderer
CC=gcc
CFLAGS=-g -D DEBUG
LDFLAGS=-lglfw -lvulkan -lcglm -lm
SRC=$(shell find src -type f -name "*.c")
OBJ=$(patsubst src/%.c, obj/%.o, $(SRC))

.PHONY: run clean

$(OUTPUTNAME): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

obj/%.o: src/%.c src/lime.h
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

run: $(OUTPUTNAME)
	./$(OUTPUTNAME)
clean:
	rm -fr obj $(OUTPUTNAME)
