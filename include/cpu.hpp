#ifndef _CPU_H
#define _CPU_H

# include "cpumask.hpp"

class cputopo {
    public:
        int thread_id;
        int core_id;
        int socket_id;
        const cpumask * thread_sibling;
        const cpumask * core_sibling;
    public:
        const cpumask * get_smt_mask() {
            return thread_sibling;
        }
        const cpumask * get_coregroup_mask() {
            return core_sibling;
        }
};

extern cputopo cpu_topology[64];
extern cpumask * cpu_online_mask;
int num_online_cpus() {
    cpumask::cpumask_weight(cpu_online_mask);
}


#endif