#include "block_allocation.h"

void
init_block_allocation_table(struct block_allocation_table *table, long size)
{
  table->size = size;
  table->free = 0;
}

long
allocate_block(struct block_allocation_table *table, long block_size)
{
  long offset;
  offset = table->free;
  table->free += block_size;
  return offset;
}

void
free_block(struct block_allocation_table *table, long offset)
{
  /* TODO Implement free list. */
}

void
destroy_block_allocation_table(struct block_allocation_table *table)
{
}
