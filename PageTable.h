#ifndef PAGE_H
#define PAGE_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    bool valid; // true if the associated page is in memory
    int frame; // reference to the memory frame
} page_table_block;

page_table_block* init_dense_page_table(unsigned int number_of_pages);

#endif
