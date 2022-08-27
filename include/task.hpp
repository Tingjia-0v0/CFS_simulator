#ifndef _TASK_H
#define _TASK_H

# include "cpumask.hpp"

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

        sched_entity() {
            weight = 0;
            runnable_weight = 0;
            on_rq = 0;
            exec_start = 0;
            sum_exec_runtime = 0;
            vruntime = 0;
            prev_sum_exec_runtime = 0;
            nr_migrations;
        }
};

class task {
    public:
        int                 pid;
        /* -1 unrunnable, 0 runnable, >0 stopped: */
        long                state;
        int                 static_prio;

        /* current CPU */
        unsigned int        cpu;
        int                 on_rq;
        sched_entity *      se;

        int                 nr_cpus_allowed;
        cpumask *           cpus_allowed;

        
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

        task(int & cur_pid, cpumask * _cpus_allowed, int normal_prio) {
            pid = cur_pid ++;
            state = TASK_NEW;
            static_prio = normal_prio;

            on_rq = 0;
            se = new sched_entity();
            cpus_allowed = _cpus_allowed;
            nr_cpus_allowed = cpumask::cpumask_weight(cpus_allowed);
        }

};

#endif