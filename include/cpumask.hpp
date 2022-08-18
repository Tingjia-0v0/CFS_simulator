#ifndef _CPUMASK_H
#define _CPUMASK_H 

#include <string.h>
#include <iostream>
#include "../lib/bitmap.hpp"

class cpumask { 
    public:
        unsigned long bits[64];

        cpumask () {
            for ( int i = 0; i < 64; i++ ) bits[i] = 0;
        }

        cpumask (int cpu) {
            for ( int i = 0; i < 64; i++ ) bits[i] = 0;
            bits[cpu] = 1;
        }

        cpumask (const cpumask * other) {
            memcpy(bits, other->bits, sizeof(unsigned long) * 64);
        }

        /* This function will return the next cpu starting from n 
         * If it reaches the end, it will return the first cpu */
        unsigned int next(int n) {
            // return 0;
            return find_next_bit(bits, 64, n + 1);
        }
        unsigned int first() {
            return find_first_bit(bits, 64);
        }
        int test_cpu(int n) {
            return test_bit(n, bits, 64);
        }
        void set(int n) {
            bits[n] = 1;
        }
        void debug_print_cpumask() {
            for (int i = 0; i < 64; i++)
                if (bits[i] != 0) std::cout << i << " ";
            std::cout << std::endl;
        }

        static unsigned int cpumask_weight(const cpumask * src) {
            return bitmap_weight(src->bits, 64);
        }
        /* return 0 if dst is empty; else return 1 */
        static int cpumask_and(cpumask * dst, const cpumask * src1, const cpumask * src2) {
            return bitmap_and(dst->bits, src1->bits, src2->bits, 64);
        }

        static int cpumask_or(cpumask * dst, const cpumask * src1, const cpumask * src2) {
            return bitmap_or(dst->bits, src1->bits, src2->bits, 64);
        }

        static int cpumask_first(const cpumask * src) {
            return find_first_bit(src->bits, 64);
        }

        static int cpumask_equal(const cpumask *src1, const cpumask * src2) {
            return bitmap_equal(src1->bits, src2->bits, 64);
        }

        
};

#endif