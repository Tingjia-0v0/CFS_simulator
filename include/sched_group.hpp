#ifndef _SCHED_GROUP_H
#define _SCHED_GROUP_H

# include "cpumask.hpp"

class sched_group_capacity {
	public: 
	/*
	 * CPU capacity of this group, SCHED_CAPACITY_SCALE being max capacity
	 * for a single CPU.
	 */
		unsigned long capacity;
		unsigned long min_capacity; /* Min per-CPU capacity in group */
		unsigned long next_update;
		int imbalance; /* XXX unrelated to capacity but shared group state */

		cpumask * span; /* balance mask */

		sched_group_capacity() {
			capacity = min_capacity = next_update = imbalance = 0;
			span = NULL;
		}


};

class sched_group {
    public:
        sched_group * next; // circular list
        unsigned int group_weight;
        sched_group_capacity *sgc;
        int asym_prefer_cpu;		/* cpu of highest priority in group */
        cpumask * span;

		sched_group() {
			next = NULL;
			group_weight = asym_prefer_cpu = 0;
			sgc = new sched_group_capacity();
			span = NULL;
		}

};

#endif
