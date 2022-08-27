#ifndef _RUNQUEUE_H
#define _RUNQUEUE_H 

# include "sched_domain.hpp"
# include "task.hpp"

extern int cur_pid;
struct sched_avg {
    unsigned long   last_update_time;
    unsigned long   load_sum;
    unsigned long   runnable_load_sum;
    unsigned long   util_sum;
    unsigned long   period_contrib;
    unsigned long   load_avg;
    unsigned long   runnable_load_avg;
    unsigned long   util_avg;
};

class cfs_rq {
    public:
        unsigned long weight;
        unsigned long runnable_weight;
        unsigned int  nr_running;

        unsigned long exec_clock;
        unsigned long min_vruntime;

        // rb_tree *       tasks_timeline;

        sched_entity    *curr, *next, *last, *skip;

        struct sched_avg avg;

        int             cpu;

        cfs_rq(int _cpu) {
            // tasks_timeline = new rb_tree();
            weight = 0;
            runnable_weight = 0;
            nr_running = 0;
            exec_clock = 0;
            min_vruntime = 0;
            curr = next = last = skip = NULL;
            avg.last_update_time = 0;
            avg.load_sum = 0;
            avg.runnable_load_sum = 0;
            avg.util_sum = 0;
            avg.period_contrib = 0;
            avg.load_avg = 0;
            avg.runnable_load_avg = 0;
            avg.util_avg = 0;
            min_vruntime = -(1LL << 20);
            cpu = _cpu;
        }
};

class rq {
    public:
        unsigned int    nr_running;
        sched_domain    *sd;
        unsigned long   cpu_capacity;
        int             cpu;

        cfs_rq *        cfs_runqueue;

        task * curr, * idle, * stop;

        rq(int _cpu) {
            nr_running = 0;
            sd = NULL;
            cpu_capacity = 0;
            cpu = _cpu;

            cfs_runqueue = new cfs_rq(_cpu);

            idle = new task(cur_pid, new cpumask());
            stop = new task(cur_pid, new cpumask());
            curr = idle;

        }
};


#endif
