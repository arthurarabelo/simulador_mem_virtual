#include "PageTable.h"
#include "utils.h"

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

dense_page_table* init_dense_page_table(unsigned int number_of_pages) {
    dense_page_table *table = (dense_page_table*) malloc(sizeof(dense_page_table));
    table->data = (page_table_block*) malloc(number_of_pages * sizeof(page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table->data[i].valid = false;
        table->data[i].frame = -1;
    }
    return table;
}

two_level_page_table* init_two_level_page_table(unsigned int number_of_pages) {
    two_level_page_table *table = (two_level_page_table*) malloc(sizeof(two_level_page_table));
    table->data = (two_level_page_table_block*) malloc(number_of_pages * sizeof(two_level_page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table->data[i].inner_table = NULL;
    }
    return table;
}

three_level_page_table* init_three_level_page_table(unsigned int number_of_pages) {
    three_level_page_table *table = (three_level_page_table*) malloc(sizeof(three_level_page_table));
    table->data = (three_level_page_table_block*) malloc(number_of_pages * sizeof(three_level_page_table_block));
    for (size_t i = 0; i < number_of_pages; i++) {
        table->data[i].inner_table = NULL;
    }
    return table;
}

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

void free_dense_page_table(page_table* table){
    dense_page_table* table_ptr = (dense_page_table*) table->table;
    free(table_ptr->data);
    free(table->table);
    free(table);
}

void free_two_level_page_table(page_table* table){
    if (table == NULL) return;

    two_level_page_table* table_ptr = (two_level_page_table*) table->table;
    if (table_ptr != NULL) {
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

void free_three_level_page_table(page_table* table){
    if (table == NULL) return;

    three_level_page_table* table_ptr = (three_level_page_table*) table->table;
    if (table_ptr != NULL) {
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

void free_inverted_page_table(page_table* table){
    inverted_page_table* table_ptr = (inverted_page_table*) table->table;
    free(table_ptr->data);
    free(table_ptr);
    free(table);
}

page_table_block* get_page(page_table* table, int32_t outer_page_addr, int32_t second_inner_page_addr, int32_t third_inner_page_addr){
    switch (table->type) {
        case DENSE_PAGE_TABLE: {
            dense_page_table* outer_table = (dense_page_table*) table->table;
            return &outer_table->data[outer_page_addr];
        }
        case TWO_LEVEL: {
            dense_page_table* dense_table_ptr;
            two_level_page_table* outer_table = (two_level_page_table*) table->table;

            if(outer_table->data[outer_page_addr].inner_table == NULL){
                outer_table->data[outer_page_addr].inner_table = init_page_table(pow(2, count_bits_unsigned(second_inner_page_addr)), DENSE_PAGE_TABLE);
            }
            dense_table_ptr = (dense_page_table*) outer_table->data[outer_page_addr].inner_table->table;
            return &dense_table_ptr->data[second_inner_page_addr];
        }
        case THREE_LEVEL: {
            dense_page_table* dense_table_ptr;
            two_level_page_table* second_inner_table;
            three_level_page_table* outer_table = (three_level_page_table*) table->table;

            if(outer_table->data[outer_page_addr].inner_table == NULL){
                outer_table->data[outer_page_addr].inner_table = init_page_table(pow(2, count_bits_unsigned(second_inner_page_addr)), TWO_LEVEL);

                second_inner_table = (two_level_page_table*) outer_table->data[outer_page_addr].inner_table->table;
                second_inner_table->data[second_inner_page_addr].inner_table = init_page_table(pow(2,count_bits_unsigned(third_inner_page_addr)), DENSE_PAGE_TABLE);
                dense_table_ptr = (dense_page_table*) second_inner_table->data[second_inner_page_addr].inner_table->table;
            }

            second_inner_table = (two_level_page_table*) outer_table->data[outer_page_addr].inner_table->table;

            if(second_inner_table->data[second_inner_page_addr].inner_table == NULL){
                second_inner_table->data[second_inner_page_addr].inner_table = init_page_table(pow(2,count_bits_unsigned(third_inner_page_addr)), DENSE_PAGE_TABLE);
            }

            dense_table_ptr = (dense_page_table*) second_inner_table->data[second_inner_page_addr].inner_table->table;

            return &dense_table_ptr->data[third_inner_page_addr];
        }
        case INVERTED: // not possible: function only called when page table is not inverted
            break;
    }
    return NULL;
}

int replace_inverted_page_table_entry(const char *algorithm, inverted_page_table *table, size_t table_size){
    if(strcmp(algorithm, "random") == 0){
        return random_replacement_inverted_table(table_size);
    } else if(strcmp(algorithm, "lru") == 0) {
        return lru_replacement_inverted_table(table, table_size);
    }
    return -1;
}

int random_replacement_inverted_table(size_t table_size){
    return random() % table_size;
}

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
