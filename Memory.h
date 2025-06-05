#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    int virtual_page; // reference to the virtual page allocated
    bool modified; // true whenever something is written into the page
    bool allocated; // true if it is not a free-frame
    int last_access_moment; // use in lru
    int access_counter;      // use in MFU

} physical_frame;

physical_frame* init_memory(unsigned int total_physical_frames);

int find_free_frame(physical_frame *memory, size_t size);

int random_replacement(size_t mem_size);

int lru_replacement(physical_frame *memory, size_t mem_size);

int mfu_replacement(physical_frame *memory, size_t mem_size);

int lfu_replacement(physical_frame *memory, size_t mem_size);

int frame_to_be_replaced(const char *algorithm, size_t mem_size, physical_frame *memory);


#endif
