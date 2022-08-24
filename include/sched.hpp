# include "sched_domain.hpp"
# include "runqueue.hpp"
# include "jiffies.hpp"

struct s_data {
	std::vector<sched_domain *> sd;
};

class sched {
    public:
        std::vector<rq *> runqueues;
        std::vector<cputopo *> cpu_topology;
        cpumask * cpu_online_mask;

        unsigned long max_load_balance_interval;
        int sched_domain_level_max;

    public:
        sched() {
            init_cpus();

            max_load_balance_interval = HZ * num_online_cpus()/10;

            for(int i = 0; i < 64; i++) {
                if (cpu_online_mask->test_cpu(i))
                    runqueues.push_back(new rq(i));
                else
                    runqueues.push_back(NULL);
            }
            sched_init_domains(cpu_online_mask);
            
        }

        void debug_sched(int _level) {
            std::cout << "Sched domains for each cpu" << std::endl;
            int cpu;
            for_each_cpu(cpu, cpu_online_mask) {
                std::cout << "cpu: " << cpu << std::endl;
                sched_domain * tmp_sd;
                for(tmp_sd = runqueues[cpu]->sd; tmp_sd; tmp_sd = tmp_sd->parent) {
                    tmp_sd->debug_sched_domain(1);
                }
            } 
        }

        void debug_cputopo() {
            std::cout << "cpu_online_mask: ";
            cpu_online_mask->debug_print_cpumask();
            std::cout << std::endl;
            for (auto & cpu : cpu_topology) {
                std::cout << cpu->thread_id << ":" << std::endl;
                std::cout << "\tcoreid: " << cpu->core_id << " thread_id: " << cpu->socket_id << std::endl;
                std::cout << "\tthread_sibling: ";
                cpu->thread_sibling->debug_print_cpumask();
                std::cout << "\tcore_sibling: ";
                cpu->core_sibling->debug_print_cpumask();
            }
            /* print cpu_online_mask and cpu_topology */
        }
    private:
        int sched_init_domains(cpumask * cpu_map) {
            sched_domain * tmp_sd;
            struct s_data d;
            for (int i = 0; i < 64; i++) d.sd.push_back(NULL);
            rq * tmp_rq = NULL;
            int cpu;
            
            for_each_cpu(cpu, cpu_map) {
                int bottom = 1;
                tmp_sd = NULL;
                for(auto &tl : default_topology) {
                    // tl with type: sched_domain_topology_level 
                    tmp_sd = new sched_domain(&tl, cpu_map, tmp_sd, cpu, cpu_online_mask, cpu_topology, sched_domain_level_max);
                    if (bottom == 1) {
                        d.sd[cpu] = tmp_sd;
                        bottom = 0;
                    }
                    if (cpumask::cpumask_equal(cpu_map, tmp_sd->span))
                        break;
                }
            }

            for_each_cpu(cpu, cpu_map) {
                for (tmp_sd = d.sd[cpu]; tmp_sd; tmp_sd = tmp_sd->parent) 
                    tmp_sd->build_sched_groups(cpu);
            }

            for (int cpu = 63; cpu >= 0 ; cpu --) {
                if (!(cpu_map->test_cpu(cpu)))
                    continue;
                
                for ( tmp_sd = d.sd[cpu]; tmp_sd; tmp_sd = tmp_sd->parent ) {
                    if (cpu == cpumask::cpumask_first(tmp_sd->groups->sgc->span))
                        update_group_capacity(cpu, tmp_sd);
                }
            }

            /* TODO: get the lock */

            for_each_cpu(cpu, cpu_map) {
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
            interval = std::min(std::max(interval, 1UL), max_load_balance_interval);
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
                min_capacity = std::min(sgc->capacity, min_capacity);
                tmp_group = tmp_group->next;
            } while (tmp_group != sd->child->groups);

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

        int num_online_cpus() {
            cpumask::cpumask_weight(cpu_online_mask);
        }

        void init_cpus() {
            for (int i = 0; i < 64; i++) cpu_topology.push_back(NULL);
            std::ifstream ifs("/users/Tingjia/CFS_simulator/arch/lscpu_parsed.json");
            json data = json::parse(ifs);
            const auto& arr = data["online_cpu_masks"]; 
            for(auto & i : arr){
                cpu_online_mask->set(i);
            }
            const auto & dict = data["cpu_topots"];
            for (auto& el : dict.items()) {
                cpu_topology[std::stoi(el.key())] = new cputopo(el.value()["thread_id"], el.value()["core_id"], el.value()["socket_id"]);
                for(auto & i: el.value()["thread_sibling"])
                    cpu_topology[std::stoi(el.key())]->thread_sibling->set(i);
                for(auto & i: el.value()["core_sibling"])
                    cpu_topology[std::stoi(el.key())]->core_sibling->set(i);
            }
        }

};