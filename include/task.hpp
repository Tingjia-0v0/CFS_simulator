#ifndef _TASK_H
#define _TASK_H

# include "cpumask.hpp"
# include "util.hpp"
# include "load.hpp"
# include "rb_tree.hpp"

/* per entity load tracking */

class task;
class sched_entity {
    public:
        unsigned long   weight;
        unsigned long   runnable_weight;
        struct rb_node  run_node;
        unsigned long   on_rq;

        unsigned long   exec_start;
        unsigned long   sum_exec_runtime;
        unsigned long   vruntime;
        unsigned long   prev_sum_exec_runtime;

        unsigned long   nr_migrations;

        /* TODO: now assume that the task is always runnable */
        sched_avg * avg;

        task * container_task;


        sched_entity(int pid) {
            weight = 0;
            runnable_weight = 0;
            on_rq = 0;
            exec_start = 0;
            sum_exec_runtime = 0;
            vruntime = 0;
            prev_sum_exec_runtime = 0;
            nr_migrations = 0;

            avg = new sched_avg();

            run_node.vruntime = 0;
            run_node.color = RB_RED;
            run_node.rb_left = run_node.rb_right = run_node.rb_parent = NULL;
            run_node.se = this;
            run_node.pid = pid;
        }

        int update_load_avg(unsigned long now, int cpu, int running) {
            runnable_weight = weight;
            if (avg->_update_load_sum(now, cpu, !!on_rq, !!on_rq, running)) {
                avg->_update_load_avg(weight, runnable_weight);
                return 1;
            }

            return 0;
        }

        void update_vruntime(unsigned long new_vruntime){
            vruntime = new_vruntime;
            run_node.vruntime = new_vruntime;
        }

        int wake_up_preempt_entity(sched_entity * se) {
            std::cout << "    current task vruntime: " << vruntime << ", "
                      << "new task vruntime" << se->vruntime << std::endl;
            long gran, vdiff = vruntime - se->vruntime;

	        if (vdiff <= 0)
		        return -1;

	        gran = wakeup_gran(se);
	        if (vdiff > gran)
		        return 1;

	        return 0;
        }

        long wakeup_gran(sched_entity * se) {
            unsigned long gran = sysctl_sched_wakeup_granularity;
            return gran;
        }


};

class task {
    public:
        sched_entity *      se;

        int                 pid;
        /* -1 unrunnable, 0 runnable, >0 stopped: */
        long                state;

        /* TODO: assume all prio is static_prio for now */
        int                 static_prio;

        cpumask *           cpus_allowed;
        int                 nr_cpus_allowed;

        /* current CPU */
        unsigned int        cpu;
        int                 on_rq;

        int                 RESCHED_FLAG;
        int                 SIGPENDING_FLAG;

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

        task(int & cur_pid, cpumask * _cpus_allowed, int nice, int nr_thread) {
           
            pid = cur_pid ++;

            se = new sched_entity(pid);

            se->container_task = this;
            state = TASK_NEW;
            
            static_prio = NICE_TO_PRIO(nice); // nice + 120

            on_rq = 0;
            cpus_allowed = _cpus_allowed;
            nr_cpus_allowed = cpumask::cpumask_weight(cpus_allowed);

            set_load_weight(false, nr_thread);
            se->runnable_weight = se->weight;

            RESCHED_FLAG = 0;
            SIGPENDING_FLAG = 0;

            init_entity_runnable_average();
        }

        int test_tsk_need_resched() {
            return RESCHED_FLAG == 1;
        }
        void set_tsk_need_resched() {
            RESCHED_FLAG = 1;
        }
        void clear_tsk_need_resched() {
            RESCHED_FLAG = 0;
        }
        int test_tsk_signal_pending() {
            return SIGPENDING_FLAG == 1;
        }

        int signal_pending_state(long state)
        {
            if (!(state & (TASK_INTERRUPTIBLE | TASK_WAKEKILL)))
                return 0;
            if (!test_tsk_signal_pending())
                return 0;

            return (state & TASK_INTERRUPTIBLE); // TODO: || __fatal_signal_pending(p);
        }

        

    private:
        void set_load_weight(bool update_load, int nr_thread) {
            int prio = static_prio - MAX_RT_PRIO; // nice + 20
            if (update_load) {

            } else {
                se->weight = sched_prio_to_weight[prio] / nr_thread;
            }
        }

        void init_entity_runnable_average() {
            se->avg->runnable_load_avg = se->avg->load_avg = (se->weight);
	        se->runnable_weight = se->weight;
        }

};

#endif