#ifndef _TIMER_H
#define _TIMER_H

# include "jiffies.hpp"

void do_timer(unsigned long ticks)
{
	jiffies += ticks;
}

#endif