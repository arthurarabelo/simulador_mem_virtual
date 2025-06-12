#include "Memory.h"
#include "PageTable.h"
#include "utils.h"
#include <stdio.h>
#include <time.h>

#define LOGS "logs/"
#define MAX_PATH_LENGTH 64
#define DEBUG_LOG "debug.log"
#define ADDR_STR_LEN 50

// Função para escrever no log de debug
void write_debug_log(FILE* debug_file, const char* message, bool debug_mode) {
    if (debug_mode) {
        fprintf(debug_file, "%s\n", "=============================");
        fprintf(debug_file, "%s\n", message);
        fprintf(debug_file, "%s\n", "=============================");
        fflush(debug_file);
    }
}

int access_counter = 0; // used in lru aglorithm (global variable)

int main(int argc, char *argv[]) {
    bool debug_mode = false;

    // Verifica se há argumento debug
    if (argc == 7 && strcmp(argv[6], "debug") == 0) {
        debug_mode = true;
    } else if (argc < 6) {
        printf("Insuficient number of arguments");
        return 1;
    }

    // arguments
    char *algorithm = argv[1];
    char *filename = argv[2];
    unsigned int page_size = atoi(argv[3]);
    unsigned int mem_size = atoi(argv[4]);
    int table_type = atoi(argv[5]);

    // Abre arquivo de debug se necessário
    FILE* debug_file = NULL;
    if (debug_mode) {
        debug_file = fopen(DEBUG_LOG, "w");
        if (!debug_file) {
            printf("Erro ao abrir arquivo de debug\n");
            debug_mode = false;
        } else {
            write_debug_log(debug_file, "Iniciando simulação de acessos à memória", true);
        }
    }

    // variables to be used all over the program
    unsigned int dirty_pages = 0;
    unsigned int page_faults = 0;
    uint32_t offset = calculateOffset(page_size << 10);
    uint32_t second_inner_table_offset, third_inner_table_offset, outer_table_offset;
    int32_t second_inner_page_addr, third_inner_page_addr, outer_page_addr;
    unsigned int total_physical_frames = mem_size / page_size;
    unsigned int number_of_pages;

    set_tables_offset(table_type, offset, &outer_table_offset, &second_inner_table_offset, &third_inner_table_offset);

    if(table_type == INVERTED){
        number_of_pages = total_physical_frames;
    } else {
        number_of_pages = pow(2, outer_table_offset);
    }

    // initialize page table and memory
    page_table* page_table = init_page_table(number_of_pages, table_type);
    physical_frame *memory = init_memory(total_physical_frames);

    if (!page_table || !memory) {
        printf("Memory allocation failed\n");
        free(page_table);
        free(memory);
        if (debug_file) fclose(debug_file);
        return 1;
    }

    char filepath[MAX_PATH_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s%s", LOGS, filename);

    uint32_t addr;
    char rw;
    FILE *file = fopen(filepath, "r");
    int mem_access = 0;

    while (fscanf(file, "%x %c", &addr, &rw) != EOF) {
        if (debug_mode) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Processando acesso: endereço=0x%x, operação=%c", addr, rw);
            write_debug_log(debug_file, log_msg, true);
        }

        /* ============= Set each page address according to the table type ============= */
        switch (table_type) {
            case DENSE_PAGE_TABLE:
                third_inner_page_addr = -1;
                second_inner_page_addr = -1;
                outer_page_addr = addr >> offset;
                mem_access++;
                break;
            case TWO_LEVEL:
                third_inner_page_addr = -1;
                second_inner_page_addr = (addr >> offset) & make_mask(second_inner_table_offset);
                outer_page_addr = (addr >> (offset + second_inner_table_offset)) & make_mask(outer_table_offset);
                mem_access += 2;
                break;
            case THREE_LEVEL:
                third_inner_page_addr = (addr >> offset) & make_mask(third_inner_table_offset);
                second_inner_page_addr = (addr >> (offset + third_inner_table_offset)) & make_mask(second_inner_table_offset);
                outer_page_addr = (addr >> (offset + second_inner_table_offset + third_inner_table_offset)) & make_mask(outer_table_offset);
                mem_access += 3;
                break;
            case INVERTED:
                third_inner_page_addr = -1;
                second_inner_page_addr = -1;
                outer_page_addr = addr >> offset;
                mem_access++;
                break;
        }
        /* ============================================================================= */

        if(table_type == INVERTED){
            inverted_page_table* table_ptr = (inverted_page_table*) page_table->table; // instantiate the table to its correct type
            inverted_page_table_block* block_ptr = NULL; // this variable will keep the associated entry
            int free_block_index = -1;
            bool page_found = false;

            if (debug_mode) {
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "Procurando página %u na tabela invertida", outer_page_addr);
                write_debug_log(debug_file, log_msg, true);
            }

            // searches for the page or for a free block
            for (size_t i = 0; i < page_table->table_size; i++) {
                if(table_ptr->data[i].page == outer_page_addr){
                    block_ptr = &table_ptr->data[i];
                    page_found = true;
                    if (debug_mode) {
                        char log_msg[256];
                        snprintf(log_msg, sizeof(log_msg), "Página encontrada no frame %zu", i);
                        write_debug_log(debug_file, log_msg, true);
                    }
                    break;
                } else if (block_ptr == NULL && table_ptr->data[i].page == -1) {
                    block_ptr = &table_ptr->data[i];
                    free_block_index = i;
                    if (debug_mode) {
                        char log_msg[256];
                        snprintf(log_msg, sizeof(log_msg), "Página não encontrada. Espaço livre no frame %zu", i);
                        write_debug_log(debug_file, log_msg, true);
                    }
                }
            }

            if(page_found){ // page is in memory
                if (debug_mode) {
                    write_debug_log(debug_file, "Hit na tabela invertida - atualizando dados de acesso", true);
                }

                // Hit: update access moment, modified bit & access counter
                (*block_ptr).last_access_moment = ++access_counter;
                (*block_ptr).access_counter++;
                if(rw == 'W'){
                    (*block_ptr).modified = true;
                }

                // update the frame attributes
                memory[(*block_ptr).frame].last_access_moment = ++access_counter;
                memory[(*block_ptr).frame].access_counter++;
                if(rw == 'W'){
                    memory[(*block_ptr).frame].modified = true;
                }
            } else if(!page_found && free_block_index != -1){ // page was not found but there is a free block
                page_faults++;
                mem_access++;

                if (debug_mode) {
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Page fault - alocando página %u no frame livre %d", outer_page_addr, free_block_index);
                    write_debug_log(debug_file, log_msg, true);
                }

                // change the page associated to the block and its other attributes
                table_ptr->data[free_block_index].page = outer_page_addr;
                table_ptr->data[free_block_index].modified = rw == 'W';
                (*block_ptr).last_access_moment = ++access_counter;
                (*block_ptr).access_counter = 1;
                (*block_ptr).frame = free_block_index;

                // update the frame attributes
                memory[free_block_index].modified = rw == 'W';
                memory[free_block_index].last_access_moment = ++access_counter;
                memory[free_block_index].access_counter = 1;

            } else { // page was not found and there is not a free block
                if (debug_mode) {
                    write_debug_log(debug_file, "Page fault - chamando algoritmo de substituição", true);
                }

                // call replacement algorithm
                int index_to_replace = replace_inverted_page_table_entry(algorithm, table_ptr, page_table->table_size);

                if (debug_mode) {
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Algoritmo selecionou frame %d para substituição", index_to_replace);
                    write_debug_log(debug_file, log_msg, true);
                }

                if (table_ptr->data[index_to_replace].modified) { // page was modified and need to be written on the disk
                    dirty_pages++;
                    if (debug_mode) {
                        write_debug_log(debug_file, "Página substituída estava modificada (dirty)", true);
                    }
                }
                table_ptr->data[index_to_replace].page = outer_page_addr; // replace the page

                // update the block attributes
                table_ptr->data[index_to_replace].last_access_moment = ++access_counter;
                table_ptr->data[index_to_replace].access_counter = 1;
                table_ptr->data[index_to_replace].modified = rw == 'W';

                // update the frame attributes
                memory[index_to_replace].modified = (rw == 'W');
                memory[index_to_replace].last_access_moment = ++access_counter;
                memory[index_to_replace].access_counter = 1;

                mem_access++;
            }
        } else {
            page_table_block* block = get_page(page_table, outer_page_addr, second_inner_page_addr, third_inner_page_addr, second_inner_table_offset, third_inner_table_offset);

            if (!(*block).valid) { // page was not yet brought to memory
                page_faults++;
                mem_access++;

                if (debug_mode) {
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Page fault - página %u-%u-%u não está na memória", outer_page_addr, second_inner_page_addr, third_inner_page_addr);
                    write_debug_log(debug_file, log_msg, true);
                }

                int ff_index = find_free_frame(memory, total_physical_frames);
                if (ff_index == -1) { // there is not a single free memory frame

                    if (debug_mode) {
                        write_debug_log(debug_file, "Memória cheia - chamando algoritmo de substituição", true);
                    }

                    // call page replacement algorithm
                    unsigned int mem_frame_to_replace = frame_to_be_replaced(algorithm, memory, total_physical_frames);

                    if (debug_mode) {
                        char log_msg[256];
                        snprintf(log_msg, sizeof(log_msg), "Algoritmo selecionou frame %u para substituição", mem_frame_to_replace);
                        write_debug_log(debug_file, log_msg, true);
                    }

                    if (memory[mem_frame_to_replace].modified) { // page was modified and need to be written on the disk
                        dirty_pages++;
                        if (debug_mode) {
                            write_debug_log(debug_file, "Página substituída estava modificada (dirty)", true);
                        }
                    }

                    memory[mem_frame_to_replace].virtual_page->valid = false; // make the old page allocated invalid
                    memory[mem_frame_to_replace].virtual_page->frame = -1; // make the frame reference to the old page allocated invalid
                    memory[mem_frame_to_replace].virtual_page = block; // allocate the new page

                    // update the frame attributes
                    memory[mem_frame_to_replace].modified = (rw == 'W');
                    memory[mem_frame_to_replace].last_access_moment = ++access_counter;
                    memory[mem_frame_to_replace].access_counter = 1;

                    (*block).frame = mem_frame_to_replace; // make the reference to the new frame where the page is allocated
                } else {
                    mem_access++;
                    if (debug_mode) {
                        char log_msg[256];
                        snprintf(log_msg, sizeof(log_msg), "Alocando página no frame livre %d", ff_index);
                        write_debug_log(debug_file, log_msg, true);
                    }

                    // update the frame attributes
                    memory[ff_index].allocated = true;
                    memory[ff_index].virtual_page = block;
                    memory[ff_index].modified = rw == 'W';
                    memory[ff_index].last_access_moment = ++access_counter;
                    memory[ff_index].access_counter = 1;

                    // update the block frame reference
                    (*block).frame = ff_index;
                }

                // block was brought into memory
                (*block).valid = true;
            } else {
                if (debug_mode) {
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "Hit - página %u-%u-%u encontrada no frame %d", outer_page_addr, second_inner_page_addr, third_inner_table_offset, (*block).frame);
                    write_debug_log(debug_file, log_msg, true);
                }

                // hit: update access moment, modified bit & number of accesses
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

    if (debug_mode) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Simulação concluída. Acessos: %d, Page Faults: %d, Dirty Pages: %d", mem_access, page_faults, dirty_pages);
        write_debug_log(debug_file, log_msg, true);
        fclose(debug_file);
    }

    // free allocated memory
    fclose(file);
    free_page_table(page_table, table_type);
    free(memory);

    return 0;
}
