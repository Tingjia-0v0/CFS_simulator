#ifndef _SCHED_DOMAIN_H
#define _SCHED_DOMAIN_H 

# include <vector>
# include "sched_group.hpp"
# include "cpu.hpp"
# include "jiffies.hpp"

#define SD_LOAD_BALANCE		0x0001	/* Do load balancing on this domain. */
#define SD_BALANCE_NEWIDLE	0x0002	/* Balance when about to become idle */
#define SD_BALANCE_EXEC		0x0004	/* Balance on exec */
#define SD_BALANCE_FORK		0x0008	/* Balance on fork, clone */
#define SD_BALANCE_WAKE		0x0010  /* Balance on wakeup */
#define SD_WAKE_AFFINE		0x0020	/* Wake task to waking CPU */
#define SD_ASYM_CPUCAPACITY	0x0040  /* Groups have different max cpu capacities */
#define SD_SHARE_CPUCAPACITY	0x0080	/* Domain members share cpu capacity */
#define SD_SHARE_POWERDOMAIN	0x0100	/* Domain members share power domain */
#define SD_SHARE_PKG_RESOURCES	0x0200	/* Domain members share cpu pkg resources */
#define SD_SERIALIZE		0x0400	/* Only a single load balancing instance */
#define SD_ASYM_PACKING		0x0800  /* Place busy groups earlier in the domain */
#define SD_PREFER_SIBLING	0x1000	/* Prefer to place tasks in a sibling domain */
#define SD_OVERLAP		0x2000	/* sched_domains of this level overlap */
#define SD_NUMA			0x4000	/* cross-node balancing */


# define SCHED_FIXEDPOINT_SHIFT		10
# define SCHED_CAPACITY_SHIFT	SCHED_FIXEDPOINT_SHIFT
# define SCHED_CAPACITY_SCALE	(1L << SCHED_CAPACITY_SHIFT)

/* TODO: Need to intialize at the main function */
int sched_domain_level_max;
unsigned long max_load_balance_interval;

class sched_domain_topology_level;

class sched_domain {
    public:
        sched_domain * parent;
        sched_domain * child;
        sched_group * groups; 
        unsigned long min_interval;
        unsigned long max_interval;
        unsigned int busy_factor;
        unsigned int imbalance_pct;
        unsigned int cache_nice_tries;
        unsigned int busy_idx;
        unsigned int idle_idx;
        unsigned int newidle_idx;
        unsigned int wake_idx;
        unsigned int forkexec_idx;
        unsigned int smt_gain;

        // remaining initialization
        int nohz_idle;			/* NOHZ IDLE status */
        int flags;			/* See SD_* */
        // remaining initialization
        int level;
        /* Runtime fields. */
        unsigned long last_balance;	/* init to jiffies. units in jiffies */
        unsigned int balance_interval;	/* initialise to 1. units in ms. */
        unsigned int nr_balance_failed; /* initialise to 0 */

        unsigned long max_newidle_lb_cost;
        unsigned long next_decay_max_lb_cost;

        unsigned int span_weight;
        cpumask * span;
        sched_domain_topology_level * topology_level;

    public:
        sched_domain(sched_domain_topology_level * tl, const cpumask * cpu_map, sched_domain * _child, int cpu);

        /* sched_group is shared by cpus in the same group on sd */

        int build_sched_groups(int cpu) {
            sched_group * first = NULL, * last = NULL;
            cpumask * covered = new cpumask();
            
            for (int i = cpu; i != cpu; i = span->next(i)) {
                sched_group * sg;
                if (covered->test_cpu(i)) 
                    continue;
                sg = get_group(i);
                cpumask::cpumask_or(covered, covered, sg->span);

                if (!first)
                    first = sg;
                if (last)
                    last->next = sg;
                last = sg;
            }

            last->next = first;
            groups = first;
            return 0;
        }

        sched_group * get_group(int cpu);

        
};

/* cpu flags functions */
static inline int cpu_smt_flags(void) {
	return SD_SHARE_CPUCAPACITY | SD_SHARE_PKG_RESOURCES;
}
static inline int cpu_core_flags(void) {
    return SD_SHARE_PKG_RESOURCES;
}
static inline int cpu_node_flags(void) {
    return 0;
}

/* cpu topology mask functions */
static inline const cpumask * cpu_smt_mask(int cpu) {
    cpumask * smt_mask = new cpumask(cpu_topology[cpu]->get_smt_mask());
    int emp = cpumask::cpumask_and(smt_mask, smt_mask, cpu_online_mask);
    return smt_mask;
}
static inline const cpumask * cpu_coregroup_mask(int cpu) {
    cpumask * coregroup_mask = new cpumask(cpu_topology[cpu]->get_coregroup_mask());
    int emp = cpumask::cpumask_and(coregroup_mask, coregroup_mask, cpu_online_mask);
    return coregroup_mask;
}
static inline const cpumask * cpu_cpu_mask(int cpu) {
    return cpu_online_mask;
}



typedef const cpumask * (*sched_domain_mask_f)(int cpu);
typedef int (*sched_domain_flags_f)(void);

class sched_domain_topology_level {
    public:
        sched_domain_mask_f     mask;
        sched_domain_flags_f    sd_flags;
        int		                flags;
        /* sched domains for per cpus */
        std::vector<sched_domain *>    sd;
        /* TODO: sched_domain_shared if they share package resource */
        // std::vector<sched_domain_shared *> sds;
        std::vector<sched_group *> sg;
        std::vector<sched_group_capacity *> sgc;

        sched_domain_topology_level(sched_domain_mask_f _mask, sched_domain_flags_f _sd_flags) {
            mask = _mask;
            sd_flags = _sd_flags;
            flags = 0;
            for (int i = 0; i < 64; i++) {
                sd.push_back(NULL);
                sg.push_back(new sched_group());
                sgc.push_back(new sched_group_capacity());
            }
        }
    /* TODO: figure out this member */
	// int		    numa_level;
};


static std::vector<sched_domain_topology_level> default_topology = {
    /* cpus sharing the same core */
	*(new sched_domain_topology_level( cpu_smt_mask, cpu_smt_flags )),
    /* cpus in the same socket */
	*(new sched_domain_topology_level( cpu_coregroup_mask, cpu_core_flags )),
    /* all online cpus */
	*(new sched_domain_topology_level( cpu_cpu_mask, cpu_node_flags ))
};


#endif