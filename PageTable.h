#ifndef PAGE_H
#define PAGE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum { DENSE_PAGE_TABLE, TWO_LEVEL, THREE_LEVEL, INVERTED } tableType;

typedef struct {
    void* table;
    tableType type;
    unsigned int table_size;
} page_table;

/* ============ BLOCKS ============ */

typedef struct {
    bool valid; // true if the associated page is in memory
    int frame; // reference to the memory frame
} page_table_block;

typedef struct {
    page_table *inner_table;
} two_level_page_table_block;

typedef struct {
    page_table *inner_table;
} three_level_page_table_block;

typedef struct {
    int frame;
    bool modified;
    int32_t page;
    int last_access_moment;
    int access_counter;
} inverted_page_table_block;

/* ================================ */

/* ============ PAGE TABLES ============ */

typedef struct {
    page_table_block* data;
} dense_page_table;

typedef struct {
    two_level_page_table_block* data;
} two_level_page_table;

typedef struct {
    three_level_page_table_block* data;
} three_level_page_table;

typedef struct {
    inverted_page_table_block* data;
    const char *algorithm;
} inverted_page_table;

/* ===================================== */

/* ============ FUNCTIONS ============ */

page_table* init_page_table(unsigned int number_of_pages, tableType type);

void free_page_table(page_table* table, tableType type);

dense_page_table* init_dense_page_table(unsigned int number_of_pages);

two_level_page_table* init_two_level_page_table(unsigned int number_of_pages);

three_level_page_table* init_three_level_page_table(unsigned int number_of_pages);

inverted_page_table* init_inverted_page_table(unsigned int number_of_pages);

void free_dense_page_table(page_table* table);

void free_two_level_page_table(page_table* table);

void free_three_level_page_table(page_table* table);

void free_inverted_page_table(page_table* table);

page_table_block* get_page(page_table* table, int32_t outer_page_addr, int32_t second_inner_page_addr, int32_t third_inner_page_addr);

int replace_inverted_page_table_entry(const char *algorithm, inverted_page_table *table, size_t table_size);

int random_replacement_inverted_table(size_t table_size);

int lru_replacement_inverted_table(inverted_page_table *table, size_t table_size);

int mfu_replacement_inverted_table(inverted_page_table *table, size_t mem_size);

int lfu_replacement_inverted_table(inverted_page_table *table, size_t mem_size);

/* =================================== */

#endif
