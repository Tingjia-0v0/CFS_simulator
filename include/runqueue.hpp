#ifndef _RUNQUEUE_H
#define _RUNQUEUE_H 

# include "sched_domain.hpp"

class rq {
    public:
        unsigned int nr_running;
        sched_domain *sd;
        unsigned long cpu_capacity;
        int cpu;

        rq(int _cpu) {
            nr_running = 0;
            sd = NULL;
            cpu_capacity = 0;
            cpu = _cpu;
        }
};


#endif
