# ifndef JIFFIES_H
# define JIFFIES_H

# include "util.hpp"

# define MAX_JIFFY_OFFSET   ((LONG_MAX >> 1)-1)

extern unsigned long jiffies;

static inline unsigned long _msecs_to_jiffies(const unsigned int m)
{
	return (m + (MSEC_PER_SEC / HZ) - 1) / (MSEC_PER_SEC / HZ);
}

unsigned long __msecs_to_jiffies(const unsigned int m)
{
	/*
	 * Negative value, means infinite timeout:
	 */
	if ((int)m < 0)
		return MAX_JIFFY_OFFSET;
	return _msecs_to_jiffies(m);
}

static unsigned long msecs_to_jiffies(const unsigned int m)
{
	return __msecs_to_jiffies(m);
}

# endif