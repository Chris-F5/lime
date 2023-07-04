#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

void
tuple_init(struct tuple *tuple, size_t memory_allocate, int elements_allocate)
{
  tuple->memory_allocated = memory_allocate;
  tuple->next_offset = 0;
  tuple->elements_allocated = elements_allocate;
  tuple->element_count = 0;
  tuple->memory = xmalloc(tuple->memory_allocated);
  tuple->offsets = xmalloc(tuple->elements_allocated * sizeof(long));
}

int
tuple_add(struct tuple *tuple, size_t size)
{
  int index;
  if (tuple->elements_allocated == tuple->element_count) {
    tuple->elements_allocated += 64;
    tuple->offsets = xrealloc(tuple->offsets, tuple->elements_allocated * sizeof(long));
  }
  if (tuple->next_offset + size > tuple->memory_allocated) {
    tuple->memory_allocated = ((tuple->next_offset + size) / 1024 + 4) * 1024;
    tuple->memory = xrealloc(tuple->memory, tuple->memory_allocated);
  }
  index = tuple->element_count++;
  tuple->offsets[index] = tuple->next_offset;
  tuple->next_offset += size;
  return index;
}

void *
tuple_get(struct tuple *tuple, int element)
{
  if (element < 0 || element >= tuple->element_count) {
    fprintf(stderr, "failed to get tuple element: out of range (%d)\n", element);
    exit(1);
  }
  return tuple->memory + tuple->offsets[element];
}

void
tuple_free(struct tuple *tuple)
{
  free(tuple->memory);
  free(tuple->offsets);
}
