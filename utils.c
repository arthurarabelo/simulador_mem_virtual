#include "utils.h"

uint32_t calculateOffset(int page_size) {
    uint32_t s, tmp;
    tmp = page_size;
    s = 0;
    while (tmp > 1) {
        tmp = tmp >> 1;
        s++;
    }
    return s;
}

int count_bits_unsigned(uint32_t num) {
    if (num == 0) return 1; // 0 ainda precisa de 1 bit para ser representado
    int bits = 0;
    while (num > 0) {
        num >>= 1;
        bits++;
    }
    return bits;
}

// Função para criar máscara de N bits
uint32_t make_mask(int bits) {
    return (1U << bits) - 1;
}
