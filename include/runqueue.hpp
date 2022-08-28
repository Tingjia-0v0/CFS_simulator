#ifndef _RUNQUEUE_H
#define _RUNQUEUE_H 

# include "sched_domain.hpp"
# include "task.hpp"
# include "util.hpp"

extern int cur_pid;


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

        void update_load_avg(sched_entity * se) {
            avg.util_avg += se->avg.util_avg;
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

            idle = new task(cur_pid, new cpumask(), 0);
            stop = new task(cur_pid, new cpumask(), 0);
            curr = idle;

        }

         void post_init_entity_util_avg(sched_entity * se) {
            long cap = (long)(SCHED_CAPACITY_SCALE - cfs_runqueue->avg.util_avg) / 2;
            if (cap > 0) {
                if (cfs_runqueue->avg.util_avg != 0) {
                    se->avg.util_avg = cfs_runqueue->avg.util_avg * se->weight;
                    se->avg.util_avg /= (cfs_runqueue->avg.load_avg + 1);

                    if(se->avg.util_avg > cap)
                        se->avg.util_avg = cap;
                }
            }
            cfs_runqueue->update_load_avg(se);
            
        }
};


#endif
