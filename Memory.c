#include "Memory.h"

// initialize memory and each frame attributes
physical_frame* init_memory(unsigned int total_physical_frames) {
    physical_frame *memory = (physical_frame*) malloc(total_physical_frames * sizeof(physical_frame));
    for (size_t i = 0; i < total_physical_frames; i++) {
        memory[i].virtual_page = NULL;
        memory[i].modified = false;
        memory[i].allocated = false;
        memory[i].last_access_moment = 0;
        memory[i].access_counter = 0;
    }
    return memory;
}

// searches for a frame not yet allocated
int find_free_frame(physical_frame *memory, size_t size){
    for (size_t i = 0; i < size; i++) {
        if(memory[i].allocated == false){
            return i;
        }
    }
    return -1;
}

// intermediary function that calls the specific replacement algorithms
unsigned int frame_to_be_replaced(const char *algorithm, physical_frame *memory, size_t mem_size){
    if(strcmp(algorithm, "random") == 0) {
        return random_replacement(mem_size);
    } else if(strcmp(algorithm, "lru") == 0) {
        return lru_replacement(memory, mem_size);
    } else if(strcmp(algorithm, "mfu") == 0) {
        return mfu_replacement(memory, mem_size);
    } else if(strcmp(algorithm, "lfu") == 0) {
        return lfu_replacement(memory, mem_size);
    }
    return -1;
}

// generates a random frame index between zero and memory size (number of pages)
unsigned int random_replacement(size_t mem_size){
    return random() % mem_size;
}

// returns the index of the frame with the lowest last_access_moment value
unsigned int lru_replacement(physical_frame *memory, size_t mem_size) {
    int lru_index = -1;
    int min_access_moment = __INT_MAX__;

    for (size_t i = 0; i < mem_size; i++) {
        if (memory[i].last_access_moment < min_access_moment) {
            min_access_moment = memory[i].last_access_moment;
            lru_index = i;
        }
    }
    return lru_index;
}

// returns the index of the frame with the highest access_counter value
unsigned int mfu_replacement(physical_frame *memory, size_t mem_size) {
    int mfu_index = -1;
    int max_access_counter = -1;

    for (size_t i = 0; i < mem_size; i++) {
        if (memory[i].access_counter > max_access_counter) {
            max_access_counter = memory[i].access_counter;
            mfu_index = i;
        }
    }
    return mfu_index;
}

// returns the index of the frame with the lowest access_counter value
unsigned int lfu_replacement(physical_frame *memory, size_t mem_size) {
    int lfu_index = -1;
    int min_access_counter = __INT_MAX__;

    for (size_t i = 0; i < mem_size; i++) {
        if (memory[i].access_counter < min_access_counter) {
            min_access_counter = memory[i].access_counter;
            lfu_index = i;
        }
    }
    return lfu_index;
}
