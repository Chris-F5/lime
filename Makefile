OUTPUTNAME = renderer
CC = gcc
CFLAGS = -g -D DEBUG -I src
LDFLAGS = -l glfw -l vulkan -l cglm -l m
SRCS = $(shell find ./src -type f -name "*.c")
HEADERS = $(shell find ./src -type f -name "*.h")
OBJS = $(patsubst ./src/%.c, obj/%.o, $(SRCS))
DEPENDS = $(patsubst ./src/%.c, obj/%.d,$(SRCS))

.PHONY: all run clean debug valgrind

REQUIREDSHADERS = target/obj.vert.spv \
				  target/obj.frag.spv \
				  target/lighting.vert.spv \
				  target/lighting.frag.spv

LIGHTINGSHADERDEFINES = -DOUTPUT_POSITION=1

all: target/$(OUTPUTNAME) $(REQUIREDSHADERS)

-include $(DEPENDS)

target/$(OUTPUTNAME): $(OBJS) $(HEADERS)
	mkdir -p target
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@ $(LDFLAGS)

target/obj.vert.spv: src/shaders/obj.vert
	mkdir -p $(dir $@)
	glslc $< -o $@ -Isrc/shaders
target/obj.frag.spv: src/shaders/obj.frag
	mkdir -p $(dir $@)
	glslc $< -o $@ -Isrc/shaders
target/lighting.vert.spv: src/shaders/lighting.vert
	mkdir -p $(dir $@)
	glslc $< -o $@ -Isrc/shaders
target/lighting.frag.spv: src/shaders/lighting.frag
	mkdir -p $(dir $@)
	glslc $< -o $@ -Isrc/shaders $(LIGHTINGSHADERDEFINES)

run: all
	cd target; ./$(OUTPUTNAME)

debug: all
	cd target; gdb $(OUTPUTNAME)

valgrind: all
	cd target; valgrind ./$(OUTPUTNAME)

clean:
	rm -fr target obj
