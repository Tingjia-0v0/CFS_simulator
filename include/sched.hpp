# ifndef _SIM_SCHED_H
# define _SIM_SCHED_H

# include "sched_domain.hpp"
# include "runqueue.hpp"
# include "jiffies.hpp"
# include "task.hpp"

class sched {
    public:
        std::vector<rq *> runqueues;
        std::vector<cputopo *> cpu_topology;
        cpumask * cpu_online_mask;

        unsigned long max_load_balance_interval;
        int sched_domain_level_max;

        int cur_pid = 0; 

    public:
        sched(const std::string & filename) {

            init_cpus(filename);

            max_load_balance_interval = HZ * num_online_cpus()/10;
            sched_domain_level_max = -1;

            for(int i = 0; i < NR_CPU; i++) {
                if (cpu_online_mask->test_cpu(i))
                    runqueues.push_back(new rq(i));
                else
                    runqueues.push_back(NULL);
            }

            sched_init_domains(cpu_online_mask);
            
        }

        void debug_sched(int _level) {
            std::cout << "Sched max level: " << sched_domain_level_max << std::endl;
            debug_cputopo();
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

        void wake_up_new_task(task * p, int cur_cpu) {
            
            std::cout << "start waking up task " << p->pid << std::endl;
            p->state = TASK_RUNNING;
            int dst_cpu = select_task_rq(p, cur_cpu);
            /* TODO: set hierachy cfs_rq statistic for the task */

            runqueues[dst_cpu]->post_init_entity_util_avg(p->se);
            return;
        }

    private:

        int select_task_rq(task * p, int cur_cpu) {
            if (p->nr_cpus_allowed > 1)
                return select_task_rq_fair(p, cur_cpu);
            return p->cpus_allowed->first();
        }

        /* TODO: ignore affine for now */
        int select_task_rq_fair(task * p, int cur_cpu) {

            sched_domain * tmp, *sd = NULL;

            for_each_domain(tmp, cur_cpu) {
                sd = tmp;
            }

            return find_idlest_cpu(sd, p, cur_cpu);
        }

        int find_idlest_cpu(sched_domain * sd, task * p, int cpu) {
            int new_cpu = cpu;
            while(sd) {
                sched_group * group;
                sched_domain * tmp;
                int weight;

                group = find_idlest_group(sd, p, cpu);

                std::cout << "idlest group at level " << sd->level << ": " << std::endl;
                if(group) group->debug_sched_group(0);
                else sd->groups->debug_sched_group(0);

                if (!group) {
                    sd = sd->child;
                    continue;
                }
                new_cpu = find_idlest_group_cpu(group, p, cpu);

                if (new_cpu == cpu) {
                    sd = sd->child;
                    continue;
                }

                cpu = new_cpu;
                weight = sd->span_weight;
                sd = NULL;
                for_each_domain(tmp, cpu) {
                    if (weight <= tmp->span_weight)
                        break;
                    sd = tmp;
                }
            }

            return new_cpu;
        }

        /* TODO: consider spare metrics */
        sched_group * find_idlest_group (sched_domain * sd, task * p, int cur_cpu) {
            sched_group * idlest = NULL, *group = sd->groups;
            unsigned long min_runnable_load = ULONG_MAX;
            unsigned long this_runnable_load = ULONG_MAX;
            unsigned long min_avg_load = ULONG_MAX, this_avg_load = ULONG_MAX;

            int imbalance_scale = 100 + (sd->imbalance_pct-100)/2;
            unsigned long imbalance = (NICE_0_LOAD) *
				(sd->imbalance_pct-100) / 100;
            do {
                
                unsigned long load, avg_load = 0, runnable_load = 0;
                int local_group = group->span->test_cpu(cur_cpu);
                int i;

                for_each_cpu(i, group->span) {
                    runnable_load += (runqueues[i])->cfs_runqueue->avg->runnable_load_avg;
                    avg_load += (runqueues[i])->cfs_runqueue->avg->load_avg;

                }

                avg_load = (avg_load * SCHED_CAPACITY_SCALE) /
					        group->sgc->capacity;
                runnable_load = (runnable_load * SCHED_CAPACITY_SCALE) /
					             group->sgc->capacity;

                if (local_group) {
                    this_runnable_load = runnable_load;
                    this_avg_load = avg_load;
                } else {
                    /* The runnable load is significantly smaller */
                    if (min_runnable_load > (runnable_load + imbalance)) {
                        min_runnable_load = runnable_load;
                        min_avg_load = avg_load;
                        idlest = group;
                    } else if ((runnable_load < (min_runnable_load + imbalance)) &&
                               (100*min_avg_load > imbalance_scale*avg_load) ) {
                        /* The runnable loads are close, consider avg_load */
                        min_avg_load = avg_load;
                        idlest = group;
                    }
                }
            } while (group = group->next, group != sd->groups);

            if (!idlest)
                return NULL;
            if (min_runnable_load > (this_runnable_load + imbalance))
                return NULL;
            if ((this_runnable_load < (min_runnable_load + imbalance)) &&
                (100*this_avg_load < imbalance_scale*min_avg_load))
                return NULL;
            return idlest;
        }

        int find_idlest_group_cpu(sched_group * sg, task *p, int cur_cpu) {
            unsigned long load, min_load = ULONG_MAX;
            int least_loaded_cpu = cur_cpu;
            int shallowest_idle_cpu = -1;
            int i;
            if (sg->group_weight == 1) return sg->span->first();

            for_each_cpu(i, sg->span) {
                if (!p->cpus_allowed->test_cpu(i)) continue;
                if (idle_cpu(i)) {
                    shallowest_idle_cpu = i;
                    break;
                }
                load = (runqueues[i])->cfs_runqueue->avg->runnable_load_avg;
                if ( load < min_load || (load == min_load && i == cur_cpu)) {
                    min_load = load;
                    least_loaded_cpu = i;
                }
            }

            if (shallowest_idle_cpu != -1) return shallowest_idle_cpu;
            return least_loaded_cpu;

        }

        int idle_cpu(int cpu) {
            rq * tmp_rq= runqueues[cpu];
            // if (tmp_rq->curr != tmp_rq->idle) return 0;
            if (tmp_rq->nr_running) return 0;
            return 1;

        }
        
        int sched_init_domains(cpumask * cpu_map) {
            sched_domain * tmp_sd;
            std::vector<sched_domain *> d;
            for (int i = 0; i < NR_CPU; i++) d.push_back(NULL);
            int cpu;
            
            for_each_cpu(cpu, cpu_map) {
                tmp_sd = NULL;
                for(auto &tl : default_topology) {
                    tmp_sd = new sched_domain(&tl, cpu_map, tmp_sd, cpu, cpu_online_mask, cpu_topology, sched_domain_level_max);
                    if (cpumask::cpumask_equal(cpu_map, tmp_sd->span))
                        break;
                }
                d[cpu] = default_topology[0].sd[cpu];
            }

            for_each_cpu(cpu, cpu_map) {
                for (tmp_sd = d[cpu]; tmp_sd; tmp_sd = tmp_sd->parent) 
                    tmp_sd->build_sched_groups(cpu);
            }

            for (int cpu = NR_CPU - 1; cpu >= 0 ; cpu --) {
                if (!(cpu_map->test_cpu(cpu)))
                    continue;
                
                for ( tmp_sd = d[cpu]; tmp_sd; tmp_sd = tmp_sd->parent ) {
                    if (cpu == cpumask::cpumask_first(tmp_sd->groups->sgc->span))
                        update_group_capacity(cpu, tmp_sd);
                }
            }

            /* TODO: get the lock */

            for_each_cpu(cpu, cpu_map) 
                cpu_attach_domain(d[cpu], cpu);

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

        void init_cpus(const std::string & filename) {
            for (int i = 0; i < NR_CPU; i++) cpu_topology.push_back(NULL);
            std::ifstream ifs(filename);
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

};

# endif