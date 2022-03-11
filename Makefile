OUTPUTNAME = renderer
CC = gcc
CFLAGS = -g -D DEBUG -I src
LDFLAGS = -l glfw -l vulkan -l cglm -l m
SRCS = $(shell find ./src -type f -name "*.c")
HEADERS = $(shell find ./src -type f -name "*.h")
OBJS = $(patsubst ./src/%.c, obj/%.o, $(SRCS))
DEPENDS = $(patsubst ./src/%.c, obj/%.d,$(SRCS))

.PHONY: all run clean debug valgrind

REQUIREDSHADERS = target/obj.vert.spv target/obj.frag.spv

all: target/$(OUTPUTNAME) $(REQUIREDSHADERS)

-include $(DEPENDS)

target/$(OUTPUTNAME): $(OBJS) $(HEADERS)
	mkdir -p target
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ $(LDFLAGS)

target/%.spv: src/shaders/% $(SHADERLIBS)
	mkdir -p $(dir $@)
	glslc $< -o $@

run: all
	cd target; ./$(OUTPUTNAME)

debug: all
	cd target; gdb $(OUTPUTNAME)

valgrind: all
	cd target; valgrind ./$(OUTPUTNAME)

clean:
	rm -fr target obj
