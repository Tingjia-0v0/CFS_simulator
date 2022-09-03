#ifndef _RUNQUEUE_H
#define _RUNQUEUE_H 

# include "sched_domain.hpp"
# include "task.hpp"
# include "util.hpp"
# include "load.hpp"

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

        sched_avg * avg;

        int             cpu;

        cfs_rq(int _cpu) {
            // tasks_timeline = new rb_tree();
            weight = 0;
            runnable_weight = 0;
            nr_running = 0;
            exec_clock = 0;
            min_vruntime = 0;
            curr = next = last = skip = NULL;
            avg = new sched_avg();
            min_vruntime = -(1LL << 20);
            cpu = _cpu;
        }

        void attach_entity_load_avg(sched_entity * se) {
            unsigned long divider = LOAD_AVG_MAX - 1024 + avg->period_contrib;

            se->avg->last_update_time = avg->last_update_time;
            se->avg->period_contrib = avg->period_contrib;
            se->avg->util_sum = se->avg->util_avg * divider;
            se->avg->load_sum = se->avg->load_avg * divider / se->weight; /* When initializing, it is weight * divider / weight */
            se->avg->runnable_load_sum = se->avg->load_sum;

            enqueue_load_avg(se);
            
            avg->util_avg += se->avg->util_avg;
            avg->util_sum += se->avg->util_sum;
        }

        /* update task and cfs's load_avg 
           TODO: now it only updates the cfs_rq's load_avg */
        void update_load_avg(sched_entity * se, int flags) {

            unsigned long now = jiffies_to_msecs(jiffies);
            if (se->avg->last_update_time && !(flags & SKIP_AGE_LOAD))
                se->update_load_avg(now, cpu, curr == se);
            
            int decayed  = update_cfs_rq_load_avg(now);

            if (!se->avg->last_update_time && (flags & DO_ATTACH)) 
                attach_entity_load_avg(se);

        }

    private:
        void enqueue_load_avg( struct sched_entity *se)
        {
            avg->load_avg += se->avg->load_avg;
            avg->load_sum += se->weight * se->avg->load_sum;
        }

        int update_cfs_rq_load_avg(unsigned long now) {
            int decayed = 0;
            decayed |= __update_load_avg_cfs_rq(now, cpu);
            return decayed;
        }

        int __update_load_avg_cfs_rq(unsigned long now, int cpu)
        {
            if (avg->_update_load_sum(now, cpu, weight, runnable_weight, curr!= NULL)) {
                avg->_update_load_avg(1, 1);
                return 1;
            }

            return 0;
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

        /*
         * util_avg = cfs_rq->util_avg / (cfs_rq->load_avg + 1) * se.load.weight
         *
         *  task  util_avg: 512, 256, 128,  64,  32,   16,    8, ...
         * cfs_rq util_avg: 512, 768, 896, 960, 992, 1008, 1016, ...
         * util_avg = running% * SCHED_CAPACITY_SCALE (1024)
         */
        void post_init_entity_util_avg(sched_entity * se) {
            long cap = (long)(SCHED_CAPACITY_SCALE - cfs_runqueue->avg->util_avg) / 2;
            if (cap > 0) {
                if (cfs_runqueue->avg->util_avg != 0) {
                    se->avg->util_avg = cfs_runqueue->avg->util_avg * se->weight;
                    se->avg->util_avg /= (cfs_runqueue->avg->load_avg + 1);

                    if(se->avg->util_avg > cap)
                        se->avg->util_avg = cap;
                } else {
                    se->avg->util_avg = cap;
                }
            }
            /* update the load_avg of cfs_runqueue */
            cfs_runqueue->update_load_avg(se, SKIP_AGE_LOAD);
            cfs_runqueue->attach_entity_load_avg(se);
            
        }
};


#endif
