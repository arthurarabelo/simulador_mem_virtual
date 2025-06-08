#include "PageTable.h"

// initialize page table
page_table* init_page_table(unsigned int number_of_pages, tableType type){
    page_table *table = (page_table*) malloc(sizeof(page_table));
    table->type = type;
    table->table_size = number_of_pages;
    switch (type) {
        case DENSE_PAGE_TABLE:
            table->table = init_dense_page_table(number_of_pages);
            break;
        case TWO_LEVEL:
            table->table = init_two_level_page_table(number_of_pages);
            break;
        case THREE_LEVEL:
            table->table = init_three_level_page_table(number_of_pages);
            break;
        case INVERTED:
            table->table = init_inverted_page_table(number_of_pages);
            break;
    }
    return table;
}

// initialize dense page table
dense_page_table* init_dense_page_table(unsigned int number_of_pages) {
    dense_page_table *table = (dense_page_table*) malloc(sizeof(dense_page_table));
    table->data = (page_table_block*) malloc(number_of_pages * sizeof(page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table->data[i].valid = false;
        table->data[i].frame = -1;
    }
    return table;
}

// initialize two level page table: initially only the outer table is allocated
two_level_page_table* init_two_level_page_table(unsigned int number_of_pages) {
    two_level_page_table *table = (two_level_page_table*) malloc(sizeof(two_level_page_table));
    table->data = (two_level_page_table_block*) malloc(number_of_pages * sizeof(two_level_page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table->data[i].inner_table = NULL;
    }
    return table;
}

// initialize three level page table: initially only the outer table is allocated
three_level_page_table* init_three_level_page_table(unsigned int number_of_pages) {
    three_level_page_table *table = (three_level_page_table*) malloc(sizeof(three_level_page_table));
    table->data = (three_level_page_table_block*) malloc(number_of_pages * sizeof(three_level_page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table->data[i].inner_table = NULL;
    }
    return table;
}

// initialize the inverted page table
inverted_page_table* init_inverted_page_table(unsigned int number_of_pages){
    inverted_page_table *table = (inverted_page_table*) malloc(sizeof(inverted_page_table));
    table->data = (inverted_page_table_block*) malloc(number_of_pages * sizeof(inverted_page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table->data[i].frame = -1;
        table->data[i].page = -1;
        table->data[i].last_access_moment = 0;
        table->data[i].modified = false;
    }
    return table;
}

// free allocated memory to the page table
void free_page_table(page_table* table, tableType type){
    if (table == NULL) return;

    switch (type) {
        case DENSE_PAGE_TABLE:
            free_dense_page_table(table);
            break;
        case TWO_LEVEL:
            free_two_level_page_table(table);
            break;
        case THREE_LEVEL:
            free_three_level_page_table(table);
            break;
        case INVERTED:
            free_dense_page_table(table);
            break;
    }
}

// free allocated memory to dense page table
void free_dense_page_table(page_table* table){
    dense_page_table* table_ptr = (dense_page_table*) table->table;
    free(table_ptr->data);
    free(table->table);
    free(table);
}

// free allocated memory to two level page table
void free_two_level_page_table(page_table* table){
    if (table == NULL) return;

    two_level_page_table* table_ptr = (two_level_page_table*) table->table;
    if (table_ptr != NULL) {
        // iterate through the outer table and frees the inner tables (dense type) if they were allocated
        for (size_t i = 0; i < table->table_size; i++) {
            if (table_ptr->data[i].inner_table != NULL) {
                free_page_table(table_ptr->data[i].inner_table, DENSE_PAGE_TABLE);
            }
        }
        free(table_ptr->data);
        free(table_ptr);
    }
    free(table);
}

// free allocated memory to three level page table
void free_three_level_page_table(page_table* table){
    if (table == NULL) return;

    three_level_page_table* table_ptr = (three_level_page_table*) table->table;
    if (table_ptr != NULL) {
        // iterate through the outer table and frees the inner tables (two level type) if they were allocated
        for (size_t i = 0; i < table->table_size; i++) {
            if (table_ptr->data[i].inner_table != NULL) {
                free_page_table(table_ptr->data[i].inner_table, TWO_LEVEL);
            }
        }
        free(table_ptr->data);
        free(table_ptr);
    }
    free(table);
}

// free allocated memory to inverted page table
void free_inverted_page_table(page_table* table){
    inverted_page_table* table_ptr = (inverted_page_table*) table->table;
    free(table_ptr->data);
    free(table_ptr);
    free(table);
}

// this function is reponsible for returning the block itself associated with the address
page_table_block* get_page(page_table* table, int32_t outer_page_addr, int32_t second_inner_page_addr, int32_t third_inner_page_addr,
                           uint32_t second_inner_table_offset, uint32_t third_inner_table_offset){

    switch (table->type) {
        // in dense tables, only the outer table address is considered, since there is only one table
        case DENSE_PAGE_TABLE: {
            dense_page_table* outer_table = (dense_page_table*) table->table;
            return &outer_table->data[outer_page_addr];
        }
        // in two level tables, the outer table and the second inner table addresses are considered, since there are two tables
        case TWO_LEVEL: {
            dense_page_table* dense_table_ptr;
            two_level_page_table* outer_table = (two_level_page_table*) table->table;

            // checks if the inner table is allocated already; if not, initialize it
            if(outer_table->data[outer_page_addr].inner_table == NULL){
                outer_table->data[outer_page_addr].inner_table = init_page_table(pow(2, second_inner_table_offset), DENSE_PAGE_TABLE);
            }
            dense_table_ptr = (dense_page_table*) outer_table->data[outer_page_addr].inner_table->table;
            return &dense_table_ptr->data[second_inner_page_addr];
        }
        // in three level tables, the outer table, the second inner table and the third inner table addresses are considered, since there are three tables
        case THREE_LEVEL: {
            dense_page_table* dense_table_ptr;
            two_level_page_table* second_inner_table;
            three_level_page_table* outer_table = (three_level_page_table*) table->table;

            // checks if the second inner table is allocated already; if not, initialize it
            if(outer_table->data[outer_page_addr].inner_table == NULL){
                outer_table->data[outer_page_addr].inner_table = init_page_table(pow(2, second_inner_table_offset), TWO_LEVEL);
                second_inner_table = (two_level_page_table*) outer_table->data[outer_page_addr].inner_table->table;

                // also initialize the third inner page, since if the second isn't, the third also is not
                second_inner_table->data[second_inner_page_addr].inner_table = init_page_table(pow(2, third_inner_table_offset), DENSE_PAGE_TABLE);
                dense_table_ptr = (dense_page_table*) second_inner_table->data[second_inner_page_addr].inner_table->table;
            }

            second_inner_table = (two_level_page_table*) outer_table->data[outer_page_addr].inner_table->table;

            // checks if the third inner table is allocated already; if not, initialize it
            if(second_inner_table->data[second_inner_page_addr].inner_table == NULL){
                second_inner_table->data[second_inner_page_addr].inner_table = init_page_table(pow(2, third_inner_table_offset), DENSE_PAGE_TABLE);
            }

            dense_table_ptr = (dense_page_table*) second_inner_table->data[second_inner_page_addr].inner_table->table;
            return &dense_table_ptr->data[third_inner_page_addr];

        }
        case INVERTED: // not possible: function only called when page table is not inverted
            break;
    }
    return NULL;
}

// intermediary function that calls the specific replacement algorithms
int replace_inverted_page_table_entry(const char *algorithm, inverted_page_table *table, size_t table_size){
    if(strcmp(algorithm, "random") == 0) {
        return random_replacement_inverted_table(table_size);
    } else if(strcmp(algorithm, "lru") == 0) {
        return lru_replacement_inverted_table(table, table_size);
    } else if(strcmp(algorithm, "mfu") == 0) {
        return mfu_replacement_inverted_table(table, table_size);
    } else if(strcmp(algorithm, "lfu") == 0) {
        return lfu_replacement_inverted_table(table, table_size);
    }
    return -1;
}

// generates a random frame index between zero and table size (number of pages)
int random_replacement_inverted_table(size_t table_size){
    return random() % table_size;
}

// returns the index of the frame with the lowest last_access_moment value
int lru_replacement_inverted_table(inverted_page_table *table, size_t table_size) {
    int lru_index = -1;
    int min_access_moment = __INT_MAX__;

    for (size_t i = 0; i < table_size; i++) {
        if (table->data[i].last_access_moment < min_access_moment) {
            min_access_moment = table->data[i].last_access_moment;
            lru_index = i;
        }
    }
    return lru_index;
}

// returns the index of the frame with the highest access_counter value
int mfu_replacement_inverted_table(inverted_page_table *table, size_t mem_size) {
    int mfu_index = -1;
    int max_access_counter = -1;

    for (size_t i = 0; i < mem_size; i++) {
        if (table->data[i].access_counter > max_access_counter) {
            max_access_counter = table->data[i].access_counter;
            mfu_index = i;
        }
    }
    return mfu_index;
}

// returns the index of the frame with the lowest access_counter value
int lfu_replacement_inverted_table(inverted_page_table *table, size_t mem_size) {
    int lfu_index = -1;
    int min_access_counter = __INT_MAX__;

    for (size_t i = 0; i < mem_size; i++) {
        if (table->data[i].access_counter < min_access_counter) {
            min_access_counter = table->data[i].access_counter;
            lfu_index = i;
        }
    }
    return lfu_index;
}

// set the each table offset accordind to its type
void set_tables_offset(tableType type, uint32_t offset,  uint32_t *outer_table_offset, uint32_t *second_inner_table_offset, uint32_t *third_inner_table_offset) {
    switch (type) {
        // dense page tables will get all the page offset
        case DENSE_PAGE_TABLE:
            *second_inner_table_offset = 0;
            *third_inner_table_offset = 0;
            *outer_table_offset = ADDRESS_SIZE - offset;
            break;
        // two level page tables will get the page offset divided into them
        case TWO_LEVEL:
            *third_inner_table_offset = 0;
            *second_inner_table_offset = (ADDRESS_SIZE - offset) / 2;
            *outer_table_offset = (ADDRESS_SIZE - offset) - *second_inner_table_offset;
            break;
        // three level page tables will get the page offset divided into them
        case THREE_LEVEL:
            *third_inner_table_offset = (ADDRESS_SIZE - offset) / 3;
            *second_inner_table_offset = (ADDRESS_SIZE - offset) / 3;
            *outer_table_offset = (ADDRESS_SIZE - offset) - *second_inner_table_offset - *third_inner_table_offset;
            break;
        // inverted page tables will get the number of frames as the offset
        case INVERTED:
            break;
    }
}
