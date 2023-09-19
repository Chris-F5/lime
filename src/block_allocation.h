struct block_allocation_table {
  long size;
  long free;
};

void init_block_allocation_table(struct block_allocation_table *table, long size);
long allocate_block(struct block_allocation_table *table, long block_size);
void free_block(struct block_allocation_table *table, long offset);
void destroy_block_allocation_table(struct block_allocation_table *table);
