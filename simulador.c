#include "Memory.h"
#include "PageTable.h"
#include "utils.h"
#include <stdio.h>

#define LOGS "logs/"
#define MAX_PATH_LENGTH 64
#define ADDRESS_SIZE 32

int access_counter = 0; // used in lru aglorithm (global variable)

void set_tables_offset(tableType type, uint32_t offset,  uint32_t *outer_table_offset, uint32_t *second_inner_table_offset, uint32_t *third_inner_table_offset) {
    switch (type) {
        case DENSE_PAGE_TABLE:
            *second_inner_table_offset = 0;
            *third_inner_table_offset = 0;
            *outer_table_offset = ADDRESS_SIZE - offset;
            break;
        case TWO_LEVEL:
            *third_inner_table_offset = 0;
            *second_inner_table_offset = (ADDRESS_SIZE - offset) / 2;
            *outer_table_offset = (ADDRESS_SIZE - offset) - *second_inner_table_offset;
            break;
        case THREE_LEVEL:
            *third_inner_table_offset = (ADDRESS_SIZE - offset) / 3;
            *second_inner_table_offset = (ADDRESS_SIZE - offset) / 3;
            *outer_table_offset = (ADDRESS_SIZE - offset) - *second_inner_table_offset - *third_inner_table_offset;
            break;
        case INVERTED:
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Insuficient number of arguments");
        return 1;
    }

    char *algorithm = argv[1];
    char *filename = argv[2];
    unsigned int page_size = atoi(argv[3]);
    unsigned int mem_size = atoi(argv[4]);
    int table_type = atoi(argv[5]);
    unsigned int page_faults = 0;
    unsigned int dirty_pages = 0;

    uint32_t offset = calculateOffset(page_size << 10);
    uint32_t second_inner_table_offset, third_inner_table_offset, outer_table_offset;
    set_tables_offset(table_type, offset, &outer_table_offset, &second_inner_table_offset, &third_inner_table_offset);

    unsigned int total_physical_frames = mem_size / page_size;

    unsigned int number_of_pages;
    if(table_type == INVERTED){
        number_of_pages = total_physical_frames;
    } else {
        number_of_pages = pow(2, outer_table_offset);
    }

    page_table* page_table = init_page_table(number_of_pages, table_type);
    physical_frame *memory = init_memory(total_physical_frames);

    if (!page_table || !memory) {
        printf("Memory allocation failed\n");
        free(page_table);
        free(memory);
        return 1;
    }

    char filepath[MAX_PATH_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%s", LOGS, filename);

    uint32_t addr, page;
    int32_t second_inner_page_addr, third_inner_page_addr, outer_page_addr;
    int mem_access = 0;
    char rw;
    FILE *file = fopen(filepath, "r");

    while (fscanf(file, "%x %c", &addr, &rw) != EOF) {
        mem_access++;
        page = addr >> offset;

        switch (table_type) {
            case DENSE_PAGE_TABLE:
                third_inner_page_addr = -1;
                second_inner_page_addr = -1;
                outer_page_addr = page;
                break;
            case TWO_LEVEL:
                third_inner_page_addr = -1;
                second_inner_page_addr = (addr >> offset) & make_mask(second_inner_table_offset);
                outer_page_addr = (addr >> (offset + second_inner_table_offset)) & make_mask(outer_table_offset);
                break;
            case THREE_LEVEL:
                third_inner_page_addr = (addr >> offset) & make_mask(third_inner_table_offset);
                second_inner_page_addr = (addr >> (offset + third_inner_table_offset)) & make_mask(second_inner_table_offset);
                outer_page_addr = (addr >> (offset + second_inner_table_offset + third_inner_table_offset)) & make_mask(outer_table_offset);
                break;
            case INVERTED:
                third_inner_page_addr = -1;
                second_inner_page_addr = -1;
                outer_page_addr = page;
                break;
        }

        if(table_type == INVERTED){
            inverted_page_table* table_ptr = (inverted_page_table*) page_table->table;
            inverted_page_table_block* block_ptr = NULL;
            int free_block_index = -1;
            bool page_found = false;

            for (size_t i = 0; i < page_table->table_size; i++) {
                if(table_ptr->data[i].page == outer_page_addr){
                    block_ptr = &table_ptr->data[i];
                    page_found = true;
                    break;
                } else if (block_ptr == NULL && table_ptr->data[i].page == -1) {
                    block_ptr = &table_ptr->data[i];
                    free_block_index = i;
                }
            }

            if(page_found){ // page is in memory
                // Hit: update access moment and modified bit
                (*block_ptr).last_access_moment = ++access_counter;
                (*block_ptr).access_counter++;
                if(rw == 'W'){
                    (*block_ptr).modified = true;
                }
            } else if(!page_found && free_block_index != -1){ // page was not found but there is a free block
                page_faults++;

                table_ptr->data[free_block_index].page = outer_page_addr;
                table_ptr->data[free_block_index].modified = rw == 'W';
                (*block_ptr).last_access_moment = ++access_counter;
                (*block_ptr).access_counter = 1;

            } else { // page was not found and there is not a free block
                // call replacement algorithm
                int index_to_replace = replace_inverted_page_table_entry(table_ptr->algorithm, table_ptr, page_table->table_size);
                if (table_ptr->data[index_to_replace].modified) { // page was modified and need to be written on the disk
                    dirty_pages++;
                }
                table_ptr->data[index_to_replace].page = outer_page_addr; // replace the page
                table_ptr->data[index_to_replace].last_access_moment = ++access_counter;
                table_ptr->data[index_to_replace].access_counter = 1;
                table_ptr->data[index_to_replace].modified = rw == 'W';
            }
        } else {
            page_table_block* block = get_page(page_table, outer_page_addr, second_inner_page_addr, third_inner_page_addr);

            if (!(*block).valid) { // page was not yet brought to memory
                page_faults++;

                int ff_index = find_free_frame(memory, total_physical_frames);
                if (ff_index == -1) { // there is not a single free memory frame

                    // call page replacement algorithm
                    int mem_frame_to_replace = frame_to_be_replaced(algorithm, memory, total_physical_frames);

                    if (memory[mem_frame_to_replace].modified) { // page was modified and need to be written on the disk
                        dirty_pages++;
                    }

                    memory[mem_frame_to_replace].virtual_page->valid = false; // make the old page allocated invalid
                    memory[mem_frame_to_replace].virtual_page->frame = -1;
                    memory[mem_frame_to_replace].virtual_page = block; // allocate the new page
                    memory[mem_frame_to_replace].modified = (rw == 'W');
                    memory[mem_frame_to_replace].last_access_moment = ++access_counter;
                    memory[mem_frame_to_replace].access_counter = 1;

                    (*block).frame = mem_frame_to_replace; // make the reference to the new frame where the page is allocated
                } else {
                    memory[ff_index].allocated = true;
                    memory[ff_index].virtual_page = block;
                    memory[ff_index].modified = rw == 'W';
                    memory[ff_index].last_access_moment = ++access_counter;
                    memory[ff_index].access_counter = 1;
                    (*block).frame = ff_index;
                }

                (*block).valid = true;
            } else {
                // Hit: update access moment and modified bit
                memory[(*block).frame].last_access_moment = ++access_counter;
                memory[(*block).frame].access_counter++;
                if(rw == 'W'){
                    memory[(*block).frame].modified = true;
                }
            }
        }
    }

    printf("Algorithm: %s\n", algorithm);
    printf("Filename: %s\n", filename);
    printf("Page size: %d\n", page_size);
    printf("Memory size: %d\n", mem_size);
    printf("Memory accesses: %d\n", mem_access);
    printf("Page faults: %d\n", page_faults);
    printf("Dirty pages: %d\n", dirty_pages);

    fclose(file);
    free_page_table(page_table, table_type);
    free(memory);

    return 0;
}
