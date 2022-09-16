#ifndef _CPUMASK_H
#define _CPUMASK_H 

#include <string.h>
#include <iostream>
#include "bitmap.hpp"
#include "util.hpp"

class cpumask { 
    public:
        unsigned long bits[NR_CPU];

        cpumask () {
            for ( int i = 0; i < NR_CPU; i++ ) bits[i] = 0;
        }

        cpumask (int cpu) {
            for ( int i = 0; i < NR_CPU; i++ ) bits[i] = 0;
            bits[cpu] = 1;
        }

        cpumask (const cpumask * other) {
            memcpy(bits, other->bits, sizeof(unsigned long) * NR_CPU);
        }

        /* This function will return the next cpu starting from n 
         * If it reaches the end, it will return the first cpu */
        unsigned int next(int n) {
            return find_next_bit(bits, NR_CPU, n);
        }

        unsigned int next_and(int n, cpumask * mask_and) {
            return find_next_bit_and(bits, mask_and->bits, NR_CPU, n);
        }

        unsigned int first() {
            return find_first_bit(bits, NR_CPU);
        }

        unsigned int first_and(cpumask * mask_and) {
            return find_first_bit_and(bits, mask_and->bits, NR_CPU);
        }
        int test_cpu(int n) {
            return test_bit(n, bits, NR_CPU);
        }
        void clear_cpu(int n) {
            bits[n] = 0;
        }
        void set(int n) {
            bits[n] = 1;
        }
        void debug_print_cpumask() {
            for (int i = 0; i < NR_CPU; i++)
                if (bits[i] != 0) std::cout << i << " ";
            std::cout << std::endl;
        }

        static unsigned int cpumask_weight(const cpumask * src) {
            return bitmap_weight(src->bits, NR_CPU);
        }
        /* return 0 if dst is empty; else return 1 */
        static int cpumask_and(cpumask * dst, const cpumask * src1, const cpumask * src2) {
            return bitmap_and(dst->bits, src1->bits, src2->bits, NR_CPU);
        }

        static int cpumask_or(cpumask * dst, const cpumask * src1, const cpumask * src2) {
            return bitmap_or(dst->bits, src1->bits, src2->bits, NR_CPU);
        }

        static int cpumask_first(const cpumask * src) {
            return find_first_bit(src->bits, NR_CPU);
        }

        static int cpumask_equal(const cpumask *src1, const cpumask * src2) {
            return bitmap_equal(src1->bits, src2->bits, NR_CPU);
        }

        static int cpumask_subset(const cpumask * src1, const cpumask * src2) {
            return bitmap_subset(src1->bits, src2->bits, NR_CPU);
        }

        
};

#endif