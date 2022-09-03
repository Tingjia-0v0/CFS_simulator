# include "jiffies.hpp"

unsigned long jiffies;


/* j = ceil m/4 */
unsigned long msecs_to_jiffies(unsigned int m)
{
	return (m + (MSEC_PER_SEC / HZ) - 1) / (MSEC_PER_SEC / HZ);
}

/* m = 4j */
unsigned long jiffies_to_msecs(unsigned int j)
{
	return (MSEC_PER_SEC / HZ) * j;
}