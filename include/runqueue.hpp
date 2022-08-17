#ifndef _RUNQUEUE_H
#define _RUNQUEUE_H 

# include "sched_domain.hpp"

class rq {
    public:
        unsigned int nr_running;
        sched_domain *sd;
        unsigned long cpu_capacity;
        int cpu;
};


#endif
