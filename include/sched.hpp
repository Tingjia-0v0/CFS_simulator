# include <vector>
# include "sched_domain.hpp"
# include "cpumask.hpp"

struct s_data {
	std::vector<sched_domain *> sd;
};

class sched {
    public:
        // struct cpumask * cpu_online_mask;
        std::vector<rq *> runqueues;
    public:
        sched() {
            sched_init_domains(cpu_online_mask);
        }
    private:
        int sched_init_domains(cpumask * cpu_map) {
            sched_domain * tmp_sd;
            struct s_data d;
            for (int i = 0; i < 64; i++) d.sd.push_back(NULL);
            rq * tmp_rq = NULL;
            
            for (int cpu = cpu_map->first(); cpu != cpu_map->first(); cpu = cpu_map->next(cpu)) {
                int bottom = 1;
                tmp_sd = NULL;
                for(auto &tl : default_topology) {
                    // tl with type: sched_domain_topology_level 
                    tmp_sd = new sched_domain(&tl, cpu_map, tmp_sd, cpu);
                    if (bottom == 1) {
                        d.sd[cpu] = tmp_sd;
                        bottom = 0;
                    }
                    if (cpumask::cpumask_equal(cpu_map, tmp_sd->span))
                        break;
                }
            }

            for (int cpu = cpu_map->first(); cpu != cpu_map->first(); cpu = cpu_map->next(cpu)) 
                for (tmp_sd = d.sd[cpu]; tmp_sd; tmp_sd = tmp_sd->parent) 
                    tmp_sd->build_sched_groups(cpu);

            for (int cpu = 63; cpu >= 0 ; cpu --) {
                if (!cpumask::cpumask_test_cpu(cpu, cpu_map))
                    continue;
                
                for ( tmp_sd = d.sd[cpu]; tmp_sd; tmp_sd = tmp_sd->parent ) {
                    if (cpu == cpumask::cpumask_first(tmp_sd->groups->sgc->span))
                        update_group_capacity(cpu, tmp_sd);
                }
            }

            /* TODO: get the lock */

            for (int cpu = cpu_map->first(); cpu != cpu_map->first(); cpu = cpu_map->next(cpu)) {
                tmp_sd = d.sd[cpu];
                cpu_attach_domain(tmp_sd, cpu);
            }
            return 0;

        }

        void update_group_capacity(int cpu, sched_domain * sd) {
            /* TODO: no need to update now 
             * since we assume no overlap domains and no asym option */
            sched_group * tmp_group;
            unsigned long capacity, min_capacity;
            unsigned long interval;
            interval = msecs_to_jiffies(sd->balance_interval);
            interval = clamp(interval, 1UL, max_load_balance_interval);
            sd->groups->sgc->next_update = jiffies + interval;

            if (!sd->child) {
                runqueues[cpu] -> cpu_capacity = SCHED_CAPACITY_SCALE;
                sd->groups->sgc->capacity = SCHED_CAPACITY_SCALE;
                sd->groups->sgc->min_capacity = SCHED_CAPACITY_SCALE;
                return;
            }

            capacity = 0;
            min_capacity = ULONG_MAX;

            tmp_group = sd->child->groups;
            do {
                sched_group_capacity *sgc = tmp_group->sgc;

                capacity += sgc->capacity;
                min_capacity = min(sgc->min_capacity, min_capacity);
                tmp_group = tmp_group->next;
            } while (tmp_group != child->groups);

            sd->groups->sgc->capacity = capacity;
            sd->groups->sgc->min_capacity = min_capacity;

        }

        void cpu_attach_domain(sched_domain *sd, int cpu) {
            /* TODO: Remove the sched domains which do not contribute to scheduling. */
            // rq->rd = rd;
            // rd->cpumaks = new cpumask(rq->cpu)
            // runqueues[cpu]->cpumask = new cpumask(sd->span);
            /* TODO: set rq online */
            runqueues[cpu]->sd = sd;
        }


};