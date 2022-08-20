#ifndef _JIFFIES_H
#define _JIFFIES_H

# include "util.hpp"

# define MAX_JIFFY_OFFSET   ((LONG_MAX >> 1)-1)

extern unsigned long jiffies;

unsigned long _msecs_to_jiffies(const unsigned int m);

unsigned long __msecs_to_jiffies(const unsigned int m);

unsigned long msecs_to_jiffies(unsigned int m);

#endif