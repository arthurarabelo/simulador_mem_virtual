#include "Memory.h"

physical_frame* init_memory(unsigned int total_physical_frames) {
    physical_frame *memory = (physical_frame*) malloc(total_physical_frames * sizeof(physical_frame));
    for (size_t i = 0; i < total_physical_frames; i++) {
        memory[i].virtual_page = -1;
        memory[i].modified = false;
        memory[i].allocated = false;
    }

    return memory;
}

int find_free_frame(physical_frame *memory, size_t size){
    for (size_t i = 0; i < size; i++) {
        if(memory[i].allocated == false){
            return i;
        }
    }
    return -1;
}

int random_replacement(size_t mem_size){
    return random() % mem_size;
}

int frame_to_be_replaced(const char *algorithm, size_t mem_size){
    if(strcmp(algorithm, "random") == 0){
        return random_replacement(mem_size);
    }
    return -1;
}
