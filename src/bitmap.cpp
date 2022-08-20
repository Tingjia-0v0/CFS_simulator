#include "bitmap.hpp"

/* treat the bitmap as a circular list and find the next setted bit
 * offset should be a number between 0 and size */
unsigned long find_next_bit(const unsigned long *addr, unsigned long size,
			    unsigned long offset)
{
	for (unsigned long i = (offset + 1) % size; i != offset; i = (i+1) % size) {
        if (addr[i] != 0) return i;
    }
    return size + 1;
}

unsigned long find_first_bit(const unsigned long *addr, unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        if (addr[i] != 0) return i;
    }
    return size + 1;
}

int test_bit(unsigned long offset, const unsigned long *addr, unsigned long size) {
    if (offset >= 0 && offset < size && addr[offset] != 0) return 1;
    return 0;
}

int bitmap_weight(const unsigned long *addr, unsigned long size) {
    int weight = 0;
    for (unsigned long i = 0; i < size; i++) {
        if (addr[i] != 0) weight ++;
    }
    return weight;
}

int bitmap_and(unsigned long *dsp, const unsigned long *src1, const unsigned long *src2, 
               unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        dsp[i] = src1[i] & src2[i];
    }
    return 1;
}

int bitmap_or(unsigned long *dsp, const unsigned long *src1, const unsigned long *src2, 
               unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        dsp[i] = src1[i] | src2[i];
    }
    return 1;
}

int bitmap_equal(const unsigned long *src1, const unsigned long *src2, 
                 unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        if ( src1[i] == 0 && src2[i] != 0 ) return 0;
        if ( src1[i] != 0 && src2[i] == 0 ) return 0;
    }
    return 1;
}
