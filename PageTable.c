#include "PageTable.h"

page_table_block* init_dense_page_table(unsigned int number_of_pages) {
    page_table_block *table = (page_table_block*) malloc(number_of_pages * sizeof(page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table[i].valid = false;
        table[i].frame = -1;
    }

    return table;
}
