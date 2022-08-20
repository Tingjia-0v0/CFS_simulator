# include "jiffies.hpp"

unsigned long jiffies;

unsigned long _msecs_to_jiffies(const unsigned int m)
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

unsigned long msecs_to_jiffies(unsigned int m)
{
	return __msecs_to_jiffies(m);
}