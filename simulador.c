#include "Memory.h"
#include "PageTable.h"
#include <math.h>
#include <stdio.h>

#define LOGS "logs/"
#define MAX_PATH_LENGTH 64
#define ADDRESS_SIZE 32

int lru_access_counter = 0; // used in lru aglorithm (global variable)

unsigned int calculateOffset(int page_size){
    unsigned int s, tmp;
    tmp = page_size;
    s = 0;
    while (tmp > 1) {
        tmp = tmp >> 1;
        s++;
    }
    return s;
}

int main(int argc, char *argv[]){
    if (argc != 5) {
        printf("Insuficient number of arguments");
        return 1;
    }

    char *algorithm = argv[1];
    char *filename = argv[2];
    unsigned int page_size = atoi(argv[3]);
    unsigned int mem_size = atoi(argv[4]);
    unsigned int page_faults = 0;
    unsigned int dirty_pages = 0;

    unsigned int offset = calculateOffset(page_size  << 10);
    unsigned int number_of_pages = pow(2, (ADDRESS_SIZE - offset));
    unsigned int total_physical_frames = mem_size / page_size;

    page_table_block *page_table = init_dense_page_table(number_of_pages);
    physical_frame *memory = init_memory(total_physical_frames);

    if(!page_table || !memory) {
        printf("Memory allocation failed\n");
        free(page_table);
        free(memory);
        return 1;
    }

    char filepath[MAX_PATH_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%s", LOGS, filename);

    unsigned int addr, page;
    int mem_access = 0;
    char rw;
    FILE* file = fopen(filepath, "r");

    while (fscanf(file, "%x %c", &addr, &rw) != EOF) {
        mem_access++;
        page = addr >> offset;
        if(!page_table[page].valid){ // page was not yet brought to memory

            page_faults++;

            int ff_index = find_free_frame(memory, total_physical_frames);
            if(ff_index == -1){ // there is not a single free memory frame
                // call page replacement algorithm
                int mem_frame_to_replace = frame_to_be_replaced(algorithm, total_physical_frames, memory);

                page_table[memory[mem_frame_to_replace].virtual_page].valid = false; // make the old page allocated invalid
                memory[mem_frame_to_replace].virtual_page =  page; // allocate the new page
                memory[mem_frame_to_replace].modified = (rw == 'W');
                memory[mem_frame_to_replace].last_access_moment = ++lru_access_counter;
                memory[mem_frame_to_replace].access_counter = 1;  // primeira vez


                page_table[page].frame = mem_frame_to_replace; // make the reference to the new frame where the page is allocated

                if(memory[mem_frame_to_replace].modified){ // page was modified and need to be written on the disk
                    dirty_pages++;
                }
            } else {
                memory[ff_index].allocated = true;
                memory[ff_index].virtual_page = page;
                memory[ff_index].modified = rw == 'W';
                memory[ff_index].last_access_moment = ++lru_access_counter;
                memory[ff_index].access_counter = 1; // primeira vez



                page_table[page].frame = ff_index;
            }

            page_table[page].valid = true;
        } else {
            // Hit: atualizar o momento de acesso e modificação
            memory[page_table[page].frame].last_access_moment = ++lru_access_counter;
            memory[page_table[page].frame].access_counter++;
            if (rw == 'W') {
                memory[page_table[page].frame].modified = true;
                }
            }
    };

    printf("Algorithm: %s\n", algorithm);
    printf("Filename: %s\n", filename);
    printf("Page size: %d\n", page_size);
    printf("Memory size: %d\n", mem_size);
    printf("Memory accesses: %d\n", mem_access);
    printf("Page faults: %d\n", page_faults);
    printf("Dirty pages: %d\n", dirty_pages);

    free(page_table);
    free(memory);

    return 0;
}
