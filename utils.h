#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <math.h>
#include <sys/types.h>
#include <stdio.h>

uint32_t calculateOffset(int page_size);
int count_bits_unsigned(uint32_t num);
uint32_t make_mask(int bits);

#endif
