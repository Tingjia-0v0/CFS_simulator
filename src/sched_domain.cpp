
# include "sched_domain.hpp"

int sched_domain_level_max;
unsigned long max_load_balance_interval;


sched_domain::sched_domain(sched_domain_topology_level * tl, const cpumask * cpu_map, sched_domain * _child, int cpu) {
    int sd_id, sd_weight, sd_flags = 0;
    /* how many cpus in this domain */
    sd_weight = cpumask::cpumask_weight(tl->mask(cpu));
    if (tl->sd_flags) sd_flags = tl->sd_flags();

    /* interval in milliseconds */
    min_interval = sd_weight;
    max_interval = 2 * sd_weight;
    busy_factor = 32;
    imbalance_pct = 125;
    cache_nice_tries = 0;
    busy_idx = 0;
    idle_idx = 0;
    newidle_idx = 0;
    wake_idx = 0;
    forkexec_idx = 0;
    smt_gain = 0;
    flags	= 1*SD_LOAD_BALANCE
            | 1*SD_BALANCE_NEWIDLE
            | 1*SD_BALANCE_EXEC
            | 1*SD_BALANCE_FORK
            | 1*SD_WAKE_AFFINE
            | sd_flags;
    last_balance = jiffies;
    balance_interval = sd_weight;
    max_newidle_lb_cost = 0;
    next_decay_max_lb_cost = jiffies;
    level = 0;
    topology_level = tl;

    child = _child;

    cpumask::cpumask_and(span, cpu_map, tl->mask(cpu));
    span_weight = cpumask::cpumask_weight(span);
    sd_id = cpumask::cpumask_first(span);

    /* TODO: initilization for further flags */

    if (child) {
        level = child->level + 1;
        sched_domain_level_max = std::max(sched_domain_level_max, level);
        child->parent = this;
    }

    /* Set the per cpu sd pointer of topology level */
    tl->sd[cpu] = this;

}
sched_group * sched_domain::get_group(int cpu) {
    sched_domain * sd = topology_level->sd[cpu];
    sched_domain * child = sd->child;
    sched_group * sg ;

    if (child) 
        cpu = cpumask::cpumask_first(child->span);
    
    sg = topology_level->sg[cpu];
    sg->sgc = topology_level->sgc[cpu];


    /* TODO: update the ref of sg and sgc for garbage collect */

    /* some duplicate work */

    if (child) {
        sg->span = new cpumask(child->span);
        sg->sgc->span = new cpumask(child->span);
    } else {
        sg->span = new cpumask(cpu);
        sg->sgc->span = new cpumask(cpu);
    }

    sg->sgc->capacity = SCHED_CAPACITY_SCALE * cpumask::cpumask_weight(sg->span);
    sg->sgc->min_capacity = SCHED_CAPACITY_SCALE;

    return sg;
}
