# ifndef _SIM_SCHED_H
# define _SIM_SCHED_H

# include "sched_domain.hpp"
# include "runqueue.hpp"
# include "jiffies.hpp"
# include "task.hpp"


struct lb_env {
	sched_domain	*sd;

	rq		        *src_rq;
	int			    src_cpu;

	int			    dst_cpu;
	rq		        *dst_rq;

	cpumask		    *dst_grpmask;
	int			    new_dst_cpu;
	int	            cpu_idle;
	long			imbalance;
	/* The set of CPUs under consideration for load-balancing */
	cpumask		    *cpus;

	unsigned int		flags;

	unsigned int		loop;
	unsigned int		loop_break;
	unsigned int		loop_max;

	int         		fbq_type;
    std::vector<task *>      tasks;
};

/*
 * sg_lb_stats - stats of a sched_group required for load_balancing
 */
struct sg_lb_stats {
	unsigned long avg_load; /*Avg load across the CPUs of the group */
	unsigned long group_load; /* Total load over the CPUs of the group */
	unsigned long sum_weighted_load; /* Weighted load of group's tasks */
	unsigned long load_per_task;
	unsigned long group_capacity;
	unsigned long group_util; /* Total utilization of the group */
	unsigned int sum_nr_running; /* Nr tasks running in the group */
	unsigned int idle_cpus;
	unsigned int group_weight;
	int group_type; /* 0: other, 1: imbalance, 2: overloaded */
	int group_no_capacity;

};

/*
 * sd_lb_stats - Structure to store the statistics of a sched_domain
 *		 during load balancing.
 */
struct sd_lb_stats {
	struct sched_group *busiest;	/* Busiest group in this sd */
	struct sched_group *local;	/* Local group in this sd */
	unsigned long total_running;
	unsigned long total_load;	/* Total load of all groups in sd */
	unsigned long total_capacity;	/* Total capacity of all groups in sd */
	unsigned long avg_load;	/* Average load across all groups in sd */

	struct sg_lb_stats busiest_stat;/* Statistics of the busiest group */
	struct sg_lb_stats local_stat;	/* Statistics of the local group */
};


class sched {
    public:
        std::vector<rq *> runqueues;
        std::vector<cputopo *> cpu_topology;
        cpumask * cpu_online_mask;

        unsigned long max_load_balance_interval;
        int sched_domain_level_max;

        int cur_pid = 0; 

        const unsigned int sched_nr_migrate_break = 32;
        const unsigned int sysctl_sched_nr_migrate = 32;

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

        void wake_up_new_task(task * p, int cur_cpu) {
            
            std::cout << ">>> start waking up task " << p->pid << std::endl;

            
            p->state = TASK_RUNNING;
            int dst_cpu = select_task_rq(p, cur_cpu);

            runqueues[dst_cpu]->preempt_disable();

            std::cout << "    choose dst_cpu: " << dst_cpu << std::endl;
            runqueues[dst_cpu]->post_init_entity_util_avg(p->se);

            runqueues[dst_cpu]->activate_task(p, ENQUEUE_NOCLOCK);

            std::cout << "    runnable_weight: " << runqueues[dst_cpu]->cfs_runqueue->runnable_weight << std::endl;
            std::cout << "    weight         : " << runqueues[dst_cpu]->cfs_runqueue->weight << std::endl;

            p->on_rq = 1;

            runqueues[dst_cpu]->check_preempt_curr(p, WF_FORK);


            // runqueues[dst_cpu]->cfs_runqueue->avg->debug_load_avg();
            // p->se->avg->debug_load_avg();
            runqueues[dst_cpu]->preempt_enable();

            std::cout << "    curr task: " << runqueues[dst_cpu]->curr->pid << " " 
                      << runqueues[dst_cpu]->cfs_runqueue->curr->container_task->pid << std::endl;
            return;
        }

        void resched_all() {
            int i;
            for_each_cpu(i, cpu_online_mask) {
                std::cout << "cpu: " << i << std::endl;
                runqueues[i]->schedule();
                std::cout << "    curr task: " << runqueues[i]->curr->pid << " ";
                if (runqueues[i]->cfs_runqueue->curr != NULL)
                    std::cout << runqueues[i]->cfs_runqueue->curr->container_task->pid << std::endl;
                else
                    std::cout << "empty cfs_runqueue" << std::endl;
            }

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

        void scheduler_tick_all_cpus() {
            int i;
            for_each_cpu(i, cpu_online_mask) {
                scheduler_tick(i);
            }
        }

        void scheduler_tick(int dst_cpu) {
            runqueues[dst_cpu]->preempt_disable();
            runqueues[dst_cpu]->task_tick_fair(runqueues[dst_cpu]->curr, 0);

            runqueues[dst_cpu]->idle_balance = runqueues[dst_cpu]->idle_cpu();

            trigger_load_balance(dst_cpu); 
        }

        void debug_rqlen() {
            std::cout << "The Queue Length of Each Runqueue" << std::endl;
            int i;
            int j;
            cpumask * covered = new cpumask();
            for_each_cpu(i, cpu_online_mask) {
                if (covered->test_cpu(i)) continue;
                covered->set(i);
                for_each_cpu(j, cpu_topology[i]->core_sibling) {
                    std::cout << runqueues[j]->nr_running << ":" << runqueues[j]->cfs_runqueue->avg->runnable_load_avg << "\t";
                    covered->set(j);
                }
                
                std::cout << std::endl;
            }
        }

    private:


        void trigger_load_balance(int dst_cpu) {
            if (jiffies >= runqueues[dst_cpu] -> next_balance) {
                run_rebalance_domains(dst_cpu);
            }
        }

        void run_rebalance_domains(int dst_cpu) {
            int cpu_idle = runqueues[dst_cpu] -> idle_balance ?
					   CPU_IDLE : CPU_NOT_IDLE;
            rebalance_domains(dst_cpu, cpu_idle);
        }

        void rebalance_domains(int dst_cpu, int cpu_idle) {
            int continue_balancing = 1;
            unsigned long interval;
            sched_domain * _sd;
            /* Earliest time when we have to do rebalance again: After 60s */
            unsigned long _next_balance = jiffies + 60 * HZ;
            int update_next_balance = 0;
            int need_serialize, need_decay = 0;
            unsigned long max_cost = 0;

            runqueues[dst_cpu] -> update_blocked_averages();

            for_each_domain(_sd, dst_cpu) {
                /*
                 * Decay the newidle max times here because this is a regular
                 * visit to all the domains. Decay ~1% per second.
                 */
                if (jiffies > _sd->next_decay_max_lb_cost) {
                    _sd->max_newidle_lb_cost =
                        (_sd->max_newidle_lb_cost * 253) / 256;
                    _sd->next_decay_max_lb_cost = jiffies + HZ;
                    need_decay = 1;
                }

                max_cost += _sd->max_newidle_lb_cost;

                if (!continue_balancing) {
                    /* need to update the cost information on the following loops */
                    if (need_decay)
                        continue;
                    break;
                }

                interval = _sd->get_sd_balance_interval(cpu_idle != CPU_IDLE);

                if ( jiffies >= _sd->last_balance + interval) {

                    if (load_balance(dst_cpu, _sd, cpu_idle, &continue_balancing)) {
                        /*
                        * The LBF_DST_PINNED logic could have changed
                        * env->dst_cpu, so we can't know our idle
                        * state even if we migrated tasks. Update it.
                        */
                        cpu_idle = runqueues[dst_cpu]->idle_cpu() ? CPU_IDLE : CPU_NOT_IDLE;
                    }
                    _sd->last_balance = jiffies;
                    interval = _sd->get_sd_balance_interval(cpu_idle != CPU_IDLE);
                }

                /* the ealist next_balance timestamp among all sds */
                if (_next_balance > _sd->last_balance + interval) {
                    _next_balance = _sd->last_balance + interval;
                    update_next_balance = 1;
                }

            }

            if (need_decay) {
                /*
                * Ensure the rq-wide value also decays but keep it at a
                * reasonable floor to avoid funnies with rq->avg_idle.
                */
                if (runqueues[dst_cpu] -> sysctl_sched_migration_cost > max_cost)
                    runqueues[dst_cpu] -> max_idle_balance_cost 
                        = runqueues[dst_cpu] -> sysctl_sched_migration_cost;
                else
                    runqueues[dst_cpu] -> max_idle_balance_cost = max_cost;
            }
            if (update_next_balance)
                runqueues[dst_cpu] -> next_balance = _next_balance;
        }

        int load_balance(int this_cpu, sched_domain *sd, 
                         int cpu_idle, int *continue_balancing) {
            int ld_moved, cur_ld_moved;
            sched_domain *sd_parent = sd->parent;
            sched_group *group;
            rq *busiest;
            cpumask *cpus = new cpumask();

            struct lb_env env = {
                .sd		    = sd,
                .src_rq     = NULL,
                .src_cpu    = -1,
                .dst_cpu	= this_cpu,
                .dst_rq		= runqueues[this_cpu],
                
                .dst_grpmask    = sd->groups->span,
                .new_dst_cpu    = -1,
                .cpu_idle		= cpu_idle,
                .imbalance      = 0,
                .cpus		    = cpus,
                .flags          = 0,
                .loop           = 0,
                .loop_break	= sched_nr_migrate_break,
                .loop_max   = 0,

                .fbq_type	= all_,
                .tasks		= {}
            };

            cpumask::cpumask_and(cpus, sd->span, cpu_online_mask);

            choose_src_rq(&env, &ld_moved, sd_parent, sd, busiest, cpus, continue_balancing, cpu_idle);

            return ld_moved;
        }

        void choose_src_rq(struct lb_env *env, int * ld_moved, 
                           sched_domain * sd_parent, sched_domain * sd,
                           rq * busiest, cpumask *cpus, int * continue_balancing, int cpu_idle) {
            if (!should_we_balance(env)) {
                continue_balancing = 0;
                out_balanced(env, sd_parent, sd, ld_moved);
                return;
            }
            sched_group * group = find_busiest_group(env);
            if (!group) {
                out_balanced(env, sd_parent, sd, ld_moved);
                return;
            }

            busiest = find_busiest_queue(env, group);
            if (!busiest) {
                out_balanced(env, sd_parent, sd, ld_moved);
                return;
            }
            env->src_cpu = busiest->cpu;
	        env->src_rq = busiest;

	        *ld_moved = 0;

            if (busiest->nr_running > 1) {
                /*
                * Attempt to move tasks. If find_busiest_group has found
                * an imbalance but busiest->nr_running <= 1, the group is
                * still unbalanced. ld_moved simply stays zero, so it is
                * correctly treated as an imbalance.
                */
                env->flags |= LBF_ALL_PINNED;
                if (sysctl_sched_nr_migrate < busiest->nr_running)
                    env->loop_max = sysctl_sched_nr_migrate;
                else
                    env->loop_max = busiest->nr_running;
            }
            do_migrate(env, ld_moved, sd_parent, sd, busiest, cpus, continue_balancing, cpu_idle);
            // tail();
        }

        void do_migrate(struct lb_env *env, int * ld_moved, 
                        sched_domain * sd_parent, sched_domain * sd,
                        rq * busiest, cpumask *cpus, int * continue_balancing, int cpu_idle) {
            if (busiest->nr_running > 1) {
                int cur_ld_moved = detach_tasks(env);
                if (cur_ld_moved) {
                    attach_tasks(env);
                    *(ld_moved) += cur_ld_moved;
                }
                if (env->flags & LBF_NEED_BREAK) {
                    env->flags &= ~LBF_NEED_BREAK;
                    do_migrate(env, ld_moved, sd_parent, sd, busiest, cpus, continue_balancing, cpu_idle);
                    return;
                }
                if ((env->flags & LBF_DST_PINNED) && env->imbalance > 0) {

                    /* Prevent to re-select dst_cpu via env's cpus */
                    env->cpus->clear_cpu(env->dst_cpu);

                    env->dst_rq	 = runqueues[env->new_dst_cpu];
                    env->dst_cpu	 = env->new_dst_cpu;
                    env->flags	&= ~LBF_DST_PINNED;
                    env->loop	 = 0;
                    env->loop_break	 = sched_nr_migrate_break;

                    /*
                    * Go back to "more_balance" rather than "redo" since we
                    * need to continue with same src_cpu.
                    */
                    do_migrate(env, ld_moved, sd_parent, sd, busiest, cpus, continue_balancing, cpu_idle);
                    return;
                }
                if (sd_parent) {
                    int *group_imbalance = &sd_parent->groups->sgc->imbalance;

                    if ((env->flags & LBF_SOME_PINNED) && env->imbalance > 0)
                        *group_imbalance = 1;
                }
                if (env->flags & LBF_ALL_PINNED) {
                    cpus->clear_cpu(busiest->cpu);
                    /*
                    * Attempting to continue load balancing at the current
                    * sched_domain level only makes sense if there are
                    * active CPUs remaining as possible busiest CPUs to
                    * pull load from which are not contained within the
                    * destination group that is receiving any migrated
                    * load.
                    */
                   // cpus is a subset of dst_grp_mask
                    if (!cpumask::cpumask_subset(cpus, env->dst_grpmask)) {
                        env->loop = 0;
                        env->loop_break = sched_nr_migrate_break;
                        choose_src_rq(env, ld_moved, sd_parent, sd, busiest, cpus, continue_balancing, cpu_idle);
                        return;
                    }
                    out_all_pinned(env, sd, ld_moved);
                    return;
                }
            }
            do_active_balance(env, sd_parent, sd, ld_moved, cpu_idle);
            return;

        }

        void do_active_balance(struct lb_env * env, sched_domain * sd_parent, sched_domain * sd, int * ld_moved, int cpu_idle) {
            int active_balance = 0;
            if (!ld_moved) {
                if (cpu_idle != CPU_NEWLY_IDLE)
                    sd->nr_balance_failed++;

                /* TODO: active_balance for asym cpu topology */
            } else
                sd->nr_balance_failed = 0;

            if (!active_balance) 
                /* We were unbalanced, so reset the balancing interval */
                sd->balance_interval = sd->min_interval;

            return;
        }

        void out_balanced(struct lb_env * env, sched_domain * sd_parent, sched_domain * sd, int * ld_moved) {
            if (sd_parent) {
                if (sd_parent->groups->sgc->imbalance)
                    sd_parent->groups->sgc->imbalance = 0;
            }
            out_all_pinned(env, sd, ld_moved);
            return;
        }

        void out_all_pinned(struct lb_env * env, sched_domain * sd, int * ld_moved) {
            sd->nr_balance_failed = 0;
            out_one_pinned(env, sd, ld_moved);
            return;
        }

        void out_one_pinned(struct lb_env * env, sched_domain * sd, int * ld_moved) {
            if (( (env->flags & LBF_ALL_PINNED) && sd->balance_interval < MAX_PINNED_INTERVAL) 
                || (sd->balance_interval < sd->max_interval))
                sd->balance_interval *= 2;

            ld_moved = 0;
            return;
        }




        int should_we_balance(struct lb_env *env)
        {
            sched_group *sg = env->sd->groups;
            int cpu, balance_cpu = -1;

            /*
            * Ensure the balancing environment is consistent; can happen
            * when the softirq triggers 'during' hotplug.
            */
            if (!env->cpus->test_cpu(env->dst_cpu))
                return 0;

            /*
            * In the newly idle case, we will allow all the cpu's
            * to do the newly idle load balance.
            */
            if (env->cpu_idle == CPU_NEWLY_IDLE)
                return 1;
            cpumask * tmp = new cpumask();
            cpumask::cpumask_and(tmp, sg->sgc->span, env->cpus);
            /* Try to find first idle cpu */
            for_each_cpu(cpu, tmp) {
                if (!idle_cpu(cpu))
                    continue;

                balance_cpu = cpu;
                break;
            }

            if (balance_cpu == -1)
                balance_cpu = cpumask::cpumask_first(sg->sgc->span);

            /*
            * First idle cpu or the first cpu(busiest) in this sched group
            * is eligible for doing load balancing at this and above domains.
            */
            return balance_cpu == env->dst_cpu;
        }


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

        rq *find_busiest_queue(struct lb_env *env, sched_group *group) {
            rq *busiest = NULL, *tmp_rq;
            unsigned long busiest_load = 0, busiest_capacity = 1;
            int i;
            for_each_cpu_and(i, group->span, env->cpus) {
                
                unsigned long capacity, wl;
                tmp_rq = runqueues[i];
                int rt = regular_;
                if (rt > env->fbq_type)
                    continue;
                capacity = tmp_rq->cpu_capacity;
                wl = tmp_rq->cfs_runqueue->avg->runnable_load_avg;

                /* this cpu only has one task and its load is too large */
                if (tmp_rq->nr_running == 1 && wl > env->imbalance)
                    continue;

                if (wl * busiest_capacity > busiest_load * capacity) {
                    busiest_load = wl;
                    busiest_capacity = capacity;
                    busiest = tmp_rq;
                }
            }
            return busiest;
        }

        sched_group *find_busiest_group(struct lb_env *env) {
            struct sg_lb_stats *local, *busiest;
            struct sd_lb_stats sds;

            init_sd_lb_stats(&sds);

            update_sd_lb_stats(env, &sds);

            local = &sds.local_stat;
            busiest = &sds.busiest_stat;

            if (!sds.busiest || busiest->sum_nr_running == 0) {
                env->imbalance = 0;
                return NULL;
            }
            sds.avg_load = (SCHED_CAPACITY_SCALE * sds.total_load)
						    / sds.total_capacity;
            if (busiest->group_type == group_imbalanced) {
                calculate_imbalance(env, &sds);
                return sds.busiest;
            }
            if (env->cpu_idle != CPU_NOT_IDLE && group_has_capacity(env, local) &&
                busiest->group_no_capacity) {
                calculate_imbalance(env, &sds);
                return sds.busiest;
            }

            if (local->avg_load >= busiest->avg_load) {
                env->imbalance = 0;
                return NULL;
            }

            if (local->avg_load >= sds.avg_load) {
                env->imbalance = 0;
                return NULL;
            }

            if (env->cpu_idle == CPU_IDLE) {
                /*
                * This cpu is idle. If the busiest group is not overloaded
                * and there is no imbalance between this and busiest group
                * wrt idle cpus, it is balanced. The imbalance becomes
                * significant if the diff is greater than 1 otherwise we
                * might end up to just move the imbalance on another group
                */
                if ((busiest->group_type != group_overloaded) &&
                        (local->idle_cpus <= (busiest->idle_cpus + 1))) {
                    env->imbalance = 0;
                    return NULL;
                }
            } else {
                /*
                * In the CPU_NEWLY_IDLE, CPU_NOT_IDLE cases, use
                * imbalance_pct to be conservative.
                */
                // imbalance_pct 110 / 117
                if (100 * busiest->avg_load <=
                        env->sd->imbalance_pct * local->avg_load) {
                    env->imbalance = 0;
                    return NULL;
                }
            }
            calculate_imbalance(env, &sds);
            return sds.busiest;

        }

        void calculate_imbalance(struct lb_env *env, struct sd_lb_stats *sds) {
            unsigned long max_pull, load_above_capacity = ~0UL;
            struct sg_lb_stats *local, *busiest;

            local = &sds->local_stat;
            busiest = &sds->busiest_stat;

            if (busiest->group_type == group_imbalanced) {
                busiest->load_per_task =
                    MIN(busiest->load_per_task, sds->avg_load);
            }

            if (busiest->avg_load <= sds->avg_load ||
                local->avg_load >= sds->avg_load) {
                env->imbalance = 0;
                return fix_small_imbalance(env, sds);
            }

            if (busiest->group_type == group_overloaded &&
                local->group_type   == group_overloaded) {
                load_above_capacity = busiest->sum_nr_running * SCHED_CAPACITY_SCALE;
                /* number of task > number of cpu
                 * load_above_capacity = extra_task * 1024 / n_cpu
                 */
                if (load_above_capacity > busiest->group_capacity) {
                    load_above_capacity -= busiest->group_capacity;
                    load_above_capacity *= NICE_0_LOAD;
                    load_above_capacity /= busiest->group_capacity;
                } else
                    load_above_capacity = ~0UL;
            }

            /*
             * busiest->avg_load - sds->avg_load: extra avg_load per cpu compared to the sd
             * load_above_capacity: extra avg_load per cpu compared to n_cpu
             */
            max_pull = MIN(busiest->avg_load - sds->avg_load, load_above_capacity);

            /* max_pull * n_cpu or max_push * n_cpu 
             * group-wise imbalance load between busiest and local
             */
            env->imbalance = MIN(
                max_pull * busiest->group_capacity,
                (sds->avg_load - local->avg_load) * local->group_capacity
            ) / SCHED_CAPACITY_SCALE;

            if (env->imbalance < busiest->load_per_task)
                return fix_small_imbalance(env, sds);
        }

        void fix_small_imbalance(struct lb_env *env, struct sd_lb_stats *sds) {
            unsigned long tmp, capa_now = 0, capa_move = 0;
            unsigned int imbn = 2;
            unsigned long scaled_busy_load_per_task;
            struct sg_lb_stats *local, *busiest;

            local = &sds->local_stat;
            busiest = &sds->busiest_stat;

            if (!local->sum_nr_running)
                local->load_per_task = runqueues[env->dst_cpu]->cpu_avg_load_per_task();
            else if (busiest->load_per_task > local->load_per_task)
                imbn = 1;

            /* load_per_task / n_cpu 
             * (load_sum)/(nr_running_per_cpu * n_cpu) / n_cpu
             */
            scaled_busy_load_per_task =
                (busiest->load_per_task * SCHED_CAPACITY_SCALE) /
                busiest->group_capacity;
            
            /* average_load per cpu in this group plus
             * load_per_task / n_cpu
             * spread the load of one task in busiest group to all cpus in local group
             */
            if (busiest->avg_load + scaled_busy_load_per_task >=
                local->avg_load + (scaled_busy_load_per_task * imbn)) {
                env->imbalance = busiest->load_per_task;
                return;
            }

            /*
             * OK, we don't have enough imbalance to justify moving tasks,
             * however we may be able to increase total CPU capacity used by
             * moving them.
             * 
             * (load_sum)/(nr_running_per_cpu * n_cpu) * n_cpu = (load_sum)/(nr_running_per_cpu)
             * avg_load * group_capacity: load_sum
             * 
             */

            capa_now += busiest->group_capacity *
                    MIN(busiest->load_per_task, busiest->avg_load);
            capa_now += local->group_capacity *
                    MIN(local->load_per_task, local->avg_load);
            capa_now /= SCHED_CAPACITY_SCALE;

            /* load_sum / n_cpu
             * load_sum / n_task / n_cpu
             * busiest->avg_load - scaled_busy_load_per_task: load per cpu after migragation
             * (busiest->avg_load - scaled_busy_load_per_task) * group_capacity: load_sum after migragation * 1024
             */
            if (busiest->avg_load > scaled_busy_load_per_task) {
                capa_move += busiest->group_capacity *
                        MIN(busiest->load_per_task,
                        busiest->avg_load - scaled_busy_load_per_task);
            }

            if (busiest->avg_load * busiest->group_capacity <
                busiest->load_per_task * SCHED_CAPACITY_SCALE) {
                tmp = (busiest->avg_load * busiest->group_capacity) /
                    local->group_capacity;
            } else {
                tmp = (busiest->load_per_task * SCHED_CAPACITY_SCALE) /
                    local->group_capacity;
            }

            capa_move += local->group_capacity *
                    MIN(local->load_per_task, local->avg_load + tmp);
            capa_move /= SCHED_CAPACITY_SCALE;

            /* Move if we gain throughput */
            if (capa_move > capa_now)
                env->imbalance = busiest->load_per_task;


        }

        void init_sd_lb_stats(struct sd_lb_stats * sds) {
            *sds = (struct sd_lb_stats){
                .busiest = NULL,
                .local = NULL,
                .total_running = 0UL,
                .total_load = 0UL,
                .total_capacity = 0UL,
                .avg_load   = 0UL,
                .busiest_stat = {
                    .avg_load = 0UL,
                    .group_load = 0UL,
                    .sum_weighted_load = 0UL,
                    .load_per_task = 0UL,
                    .group_capacity = 0UL,
                    .group_util = 0UL,
                    .sum_nr_running = 0,
                    .idle_cpus = 0,
                    .group_weight = 0,
                    .group_type = group_other,
                    .group_no_capacity = 0,
                },
                .local_stat = {
                    .avg_load = 0UL,
                    .group_load = 0UL,
                    .sum_weighted_load = 0UL,
                    .load_per_task = 0UL,
                    .group_capacity = 0UL,
                    .group_util = 0UL,
                    .sum_nr_running = 0,
                    .idle_cpus = 0,
                    .group_weight = 0,
                    .group_type = group_other,
                    .group_no_capacity = 0
                },
            };
        }

        void update_sd_lb_stats(struct lb_env * env, struct sd_lb_stats * sds) {
            sched_domain *child = env->sd->child;
            sched_group *sg = env->sd->groups;
            struct sg_lb_stats *local = &sds->local_stat;
            struct sg_lb_stats tmp_sgs;
            int load_idx, prefer_sibling = 0;
            bool overload = false;
            int i = 0;

            /* TODO: set reasonable prefer_sibling for different sd level */
            // if (child && child->flags & SD_PREFER_SIBLING)
            //     prefer_sibling = 1;
            // load_idx = get_sd_load_idx(env->sd, env->cpu_idle);

            do {
                struct sg_lb_stats *sgs = &tmp_sgs;
                int local_group;

                local_group = sg->span->test_cpu(env->dst_cpu);

                if (local_group) {
                    sds->local = sg;
                    sgs = local;
                }
                update_sg_lb_stats(env, sg, local_group, sgs, &overload);
                if (local_group) {
                    sds->total_running += sgs->sum_nr_running;
                    sds->total_load += sgs->group_load;
                    sds->total_capacity += sgs->group_capacity;
                    sg = sg->next;
                    continue;
                }
                
                if (sds->local && group_has_capacity(env, local) &&
                    (sgs->sum_nr_running > local->sum_nr_running + 1)) {
                    
                    sgs->group_no_capacity = 1;
                    sgs->group_type = group_classify(sg, sgs);
                }

                if (update_sd_pick_busiest(env, sds, sg, sgs)) {
                    sds->busiest = sg;
                    sds->busiest_stat = *sgs;
                }

                sds->total_running += sgs->sum_nr_running;
                sds->total_load += sgs->group_load;
                sds->total_capacity += sgs->group_capacity;
                sg = sg->next;
            } while (sg != env->sd->groups);

        } 

        void update_sg_lb_stats(struct lb_env *env,
                                struct sched_group *group,
                                int local_group, struct sg_lb_stats *sgs,
                                bool *overload) {
            unsigned long load;
            int i, nr_running;

            memset(sgs, 0, sizeof(sg_lb_stats));

            for_each_cpu_and(i, group->span, env->cpus) {
                load = runqueues[i]->cfs_runqueue->avg->runnable_load_avg;
                
                unsigned long util = runqueues[i]->cfs_runqueue->avg->util_avg; // 1024 * runnable %
	            unsigned long capacity = SCHED_CAPACITY_SCALE;                  // 1024

                sgs->group_load += load;
                sgs->group_util += (util >= capacity) ? capacity : util;
                sgs->sum_nr_running += runqueues[i]->cfs_runqueue->h_nr_running;

                nr_running = runqueues[i]->nr_running;
                if (nr_running > 1)
                    *overload = true;
                sgs->sum_weighted_load += runqueues[i]->cfs_runqueue->avg->runnable_load_avg;

                if (!nr_running && idle_cpu(i))
                    sgs->idle_cpus++;
            }
            sgs->group_capacity = group->sgc->capacity;
            sgs->avg_load = (sgs->group_load*SCHED_CAPACITY_SCALE) / sgs->group_capacity; // average load each cpu

            if (sgs->sum_nr_running)
                sgs->load_per_task = sgs->sum_weighted_load / sgs->sum_nr_running;

            sgs->group_weight = group->group_weight;

            sgs->group_no_capacity = group_is_overloaded(env, sgs);
            sgs->group_type = group_classify(group, sgs);

        }

        bool update_sd_pick_busiest(struct lb_env *env,
				   struct sd_lb_stats *sds,
				   struct sched_group *sg,
				   struct sg_lb_stats *sgs)
        {
            struct sg_lb_stats *busiest = &sds->busiest_stat;

            if (sgs->group_type > busiest->group_type) {
                return true;
            }

            if (sgs->group_type < busiest->group_type) {
                return false;
            }

            if (sgs->avg_load <= busiest->avg_load) {
                return false;
            }
                
            return true;
        }

        bool group_is_overloaded(struct lb_env *env, struct sg_lb_stats *sgs)
        {
            if (sgs->sum_nr_running <= sgs->group_weight)
                return false;

            if ((sgs->group_capacity * 100) <
                    (sgs->group_util * env->sd->imbalance_pct))
                return true;

            return false;
        }

        bool
        group_has_capacity(struct lb_env *env, struct sg_lb_stats *sgs)
        {
            if (sgs->sum_nr_running < sgs->group_weight)
                return true;

            if ((sgs->group_capacity * 100) >
                    (sgs->group_util * env->sd->imbalance_pct))
                return true;

            return false;
        }

        int group_classify(struct sched_group *group,
			  struct sg_lb_stats *sgs)
        {
            if (sgs->group_no_capacity)
                return group_overloaded;

            if (group->sgc->imbalance)
                return group_imbalanced;

            return group_other;
        }

        int find_idlest_cpu(sched_domain * sd, task * p, int cpu) {
            int new_cpu = cpu;
            while(sd) {
                sched_domain * tmp;
                int weight;

                sched_group * group = find_idlest_group(sd, p, cpu);
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

                /* find the child sd containing new cpu */
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
                
                unsigned long avg_load = 0, runnable_load = 0;
                int local_group = group->span->test_cpu(cur_cpu);
                int i;

                for_each_cpu(i, group->span) {
                    runnable_load += (runqueues[i])->cfs_runqueue->avg->runnable_load_avg;
                    avg_load += (runqueues[i])->cfs_runqueue->avg->load_avg;

                }

                /* average load on cpu level */
                avg_load = (avg_load * SCHED_CAPACITY_SCALE) / group->sgc->capacity;
                runnable_load = (runnable_load * SCHED_CAPACITY_SCALE) / group->sgc->capacity;

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
                    return i;
                }
                load = (runqueues[i])->cfs_runqueue->avg->runnable_load_avg;
                if ( load < min_load || (load == min_load && i == cur_cpu)) {
                    min_load = load;
                    least_loaded_cpu = i;
                }
            }

            return least_loaded_cpu;

        }

        int idle_cpu(int cpu) {
            if (runqueues[cpu]->curr != runqueues[cpu]->idle) return 0;
            if (runqueues[cpu]->nr_running) return 0;
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
                        init_sched_groups_capacity(cpu, tmp_sd);
                }
            }

            /* TODO: get the lock */

            for_each_cpu(cpu, cpu_map) 
                cpu_attach_domain(d[cpu], cpu);

            return 0;
        }
        void init_sched_groups_capacity(int cpu, sched_domain *sd)
        {
            sched_group *sg = sd->groups;
            do {
                sg->group_weight = cpumask::cpumask_weight(sg->span);

                sg = sg->next;
            } while (sg != sd->groups);

            if (cpu != cpumask::cpumask_first(sg->sgc->span))
                return;

            update_group_capacity(cpu,sd);
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
            runqueues[cpu]->sd = sd;
        }

        int num_online_cpus() {
            return cpumask::cpumask_weight(cpu_online_mask);
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

        int can_migrate_task(task * p, struct lb_env *env) {
            if (!p->cpus_allowed->test_cpu(env->dst_cpu)) {
                int cpu;
                env->flags |= LBF_SOME_PINNED;
                if (env->cpu_idle == CPU_NEWLY_IDLE || (env->flags & LBF_DST_PINNED))
                    return 0;
                for_each_cpu_and(cpu, env->dst_grpmask, env->cpus) {
                    if (p->cpus_allowed->test_cpu(cpu)) {
                        env->flags |= LBF_DST_PINNED;
                        env->new_dst_cpu = cpu;
                        break;
                    }
                }

                return 0;

            }

            env->flags &= ~LBF_ALL_PINNED;

            if (p == env->src_rq->curr) {
                return 0;
            }
            return 1;
        }

        void detach_task(task *p, struct lb_env *env) {
            p->on_rq = TASK_ON_RQ_MIGRATING;
            env->src_rq->deactivate_task(p, DEQUEUE_NOCLOCK);
            set_task_cpu(p, env->dst_cpu);
        }

        void set_task_cpu(task * p, int new_cpu) {
            if(p->cpu != new_cpu) {
                // if (p->sched_class->migrate_task_rq)
                runqueues[p->cpu]->migrate_task_rq(p);
                p->se->nr_migrations++;
            }
            p->cpu = new_cpu;
        }

        int detach_tasks(struct lb_env *env) {
            task * p;
            unsigned long load;
            int detached = 0;

            if (env->imbalance <= 0)
                return 0;

            while (!env->src_rq->cfs_tasks.empty()) {
                if (env->cpu_idle != CPU_NOT_IDLE && env->src_rq->nr_running <= 1)
                    break;
                p = env->src_rq->cfs_tasks.back();

                env->loop ++;

                if (env->loop > env->loop_max)
                    break;

                if (env->loop > env->loop_break) {
                    env->loop_break += sched_nr_migrate_break;
                    env->flags |= LBF_NEED_BREAK;
                    break;
                }

                if (!can_migrate_task(p, env)){
                    env->src_rq->move_to_front_cfs_tasks(p);
                    continue;
                }

                load = p->se->avg->load_avg;

                if (load < 16 && !env->sd->nr_balance_failed) {
                    env->src_rq->move_to_front_cfs_tasks(p);
                    continue;
                }

                if ((load / 2) > env->imbalance) {
                    env->src_rq->move_to_front_cfs_tasks(p);
                    continue;
                }

                detach_task(p, env);
                env->tasks.insert(env->tasks.begin(), p);
                detached++;
                env->imbalance -= load;
                if (env->cpu_idle == CPU_NEWLY_IDLE)
                    break;
                if (env->imbalance <= 0)
                    break;

            }
            return detached;
        }

        void attach_task(struct rq *new_rq, task *p)
        {

            new_rq->activate_task(p, ENQUEUE_NOCLOCK);
            p->on_rq = TASK_ON_RQ_QUEUED;
            new_rq->check_preempt_curr(p, 0);
        }
        
        void attach_tasks(struct lb_env *env) {
            task * p;
            while(!env->tasks.empty()) {
                p = *(env->tasks.begin());
                auto it = env->tasks.begin();
                for(; it != env->tasks.end(); it++) {
                    if(*it == p ) {
                        env->tasks.erase(it);
                        break;
                    }
                }
                attach_task(env->dst_rq, p);
            }
        }
};

# endif