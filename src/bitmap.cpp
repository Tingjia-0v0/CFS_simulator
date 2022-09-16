#include "bitmap.hpp"
#include <iostream>

/* treat the bitmap as a circular list and find the next setted bit
 * offset should be a number between 0 and size */
long find_next_bit(unsigned long *addr, unsigned long size,
			    long offset)
{
	for (long i = (offset + 1) % size; i != offset; i = (i+1) % size) {
        if (addr[i] != 0) return i;
    }
    return -1;
}

long find_next_bit_and(unsigned long *addr1, unsigned long *addr2, 
                       unsigned long size, long offset) {
    for (long i = (offset + 1) % size; i != offset; i = (i+1) % size) {
        if (addr1[i] != 0 && addr2[i] != 0) return i;
    }
    return -1;
}

long find_first_bit(const unsigned long *addr, unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        if (addr[i] != 0) return i;
    }
    return -1;
}

long find_first_bit_and(const unsigned long *addr1, const unsigned long *addr2,
                        unsigned long size) {
    for (unsigned long i = 0; i < size; i++) {
        if (addr1[i] != 0 && addr2[i] != 0) return i;
    }
    return -1;
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

/* return 1 if src1 is a subset of src2 */
int bitmap_subset(const unsigned long *src1, const unsigned long *src2, 
                  unsigned long size) {
    int r = 1;
    for (unsigned long i = 0; i < size; i++) {
        if (src1[i] != 0 && src2[i] == 0) r = 0;
    }
    return r;
}
