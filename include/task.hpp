#ifndef _TASK_H
#define _TASK_H

# include "cpumask.hpp"
# include "util.hpp"

/* per entity load tracking */

class sched_entity {
    public:
        unsigned long   weight;
        unsigned long   runnable_weight;
        // rb_node *       run_node;
        unsigned long   on_rq;

        unsigned long   exec_start;
        unsigned long   sum_exec_runtime;
        unsigned long   vruntime;
        unsigned long   prev_sum_exec_runtime;

        unsigned long   nr_migrations;

        /* TODO: now assume that the task is always runnable */
        struct sched_avg avg;


        sched_entity() {
            weight = 0;
            runnable_weight = 0;
            on_rq = 0;
            exec_start = 0;
            sum_exec_runtime = 0;
            vruntime = 0;
            prev_sum_exec_runtime = 0;
            nr_migrations = 0;
            
            
        }

};

class task {
    public:
        int                 pid;
        /* -1 unrunnable, 0 runnable, >0 stopped: */
        long                state;

        /* TODO: assume all prio is static_prio for now */
        int                 static_prio;

        /* current CPU */
        unsigned int        cpu;
        int                 on_rq;
        sched_entity *      se;

        int                 nr_cpus_allowed;
        cpumask *           cpus_allowed;

        const int sched_prio_to_weight[40] = {
            /* -20 */     88761,     71755,     56483,     46273,     36291,
            /* -15 */     29154,     23254,     18705,     14949,     11916,
            /* -10 */      9548,      7620,      6100,      4904,      3906,
            /*  -5 */      3121,      2501,      1991,      1586,      1277,
            /*   0 */      1024,       820,       655,       526,       423,
            /*   5 */       335,       272,       215,       172,       137,
            /*  10 */       110,        87,        70,        56,        45,
            /*  15 */        36,        29,        23,        18,        15,
        };
        
    /* TODO: other members
        * thread_info
        * stack
        * usage
        * flags
        * ptrace
        * wake_entry
        * on_cpu
        * wakee_flips
        * wakee_flip_decay_ts
        * last_wakee
        * wake_cpu
        * 
        * sched_class
        * 
        * sched_task_group
        * dl
        * 
        * preempt_notifiers
        * 
        * policy
        * 
        * nr_cpus_allowed
        * cpus_allowed
        * 
        * rcus
        * 
        * tasks
        * 
        * pushable_tasks
        * 
        * pushable_dl_tasks
        * 
        * mm
        * active_mm
        * 
        * vmacache
        * 
        * ...
    */

        task(int & cur_pid, cpumask * _cpus_allowed, int nice) {
            pid = cur_pid ++;
            state = TASK_NEW;
            
            se = new sched_entity();
            static_prio = NICE_TO_PRIO(nice);

            set_load_weight(false);
            se->runnable_weight = se->weight;

            on_rq = 0;
            
            cpus_allowed = _cpus_allowed;
            nr_cpus_allowed = cpumask::cpumask_weight(cpus_allowed);
        }

    private:
        void set_load_weight(bool update_load) {
            int prio = static_prio - MAX_RT_PRIO;
            if (update_load) {

            } else {
                se->weight = sched_prio_to_weight[prio];
            }
        }

};

#endif