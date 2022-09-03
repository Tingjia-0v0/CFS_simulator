#ifndef _JIFFIES_H
#define _JIFFIES_H

# include "util.hpp"

# define MAX_JIFFY_OFFSET   ((LONG_MAX >> 1)-1)

extern unsigned long jiffies;

unsigned long msecs_to_jiffies(unsigned int m);

unsigned long jiffies_to_msecs(unsigned int j);


#endif