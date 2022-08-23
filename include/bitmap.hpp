#ifndef _BITMAP_H
#define _BITMAP_H

/* treat the bitmap as a circular list and find the next setted bit
 * offset should be a number between 0 and size */
long find_next_bit(unsigned long *addr, unsigned long size,
			    long offset);

long find_first_bit(const unsigned long *addr, unsigned long size);

int test_bit(unsigned long offset, const unsigned long *addr, unsigned long size);

int bitmap_weight(const unsigned long *addr, unsigned long size);

int bitmap_and(unsigned long *dsp, const unsigned long *src1, const unsigned long *src2, 
               unsigned long size);

int bitmap_or(unsigned long *dsp, const unsigned long *src1, const unsigned long *src2, 
               unsigned long size);

int bitmap_equal(const unsigned long *src1, const unsigned long *src2, 
                 unsigned long size);

#endif