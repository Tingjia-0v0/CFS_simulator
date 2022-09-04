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
        unsigned int  h_nr_running;

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

        void enqueue_entity(sched_entity * se, int flags) {
            bool renorm = !(flags & ENQUEUE_WAKEUP) || (flags & ENQUEUE_MIGRATED);
            bool if_curr = curr == se;

            if (renorm && if_curr)
                se->vruntime += min_vruntime;

            update_curr();

            if (renorm && !if_curr) 
                se->vruntime += min_vruntime;

            update_load_avg(se, UPDATE_TG | DO_ATTACH);
            enqueue_runnable_load_avg(se);

            /*
             * TODO:
             * if (flags & ENQUEUE_WAKEUP)
             *     place_entity(cfs_rq, se, 0);
             */
            
            if (!if_curr)
                __enqueue_entity(se);

            se->on_rq = 1;
        }

    private:
        void enqueue_load_avg(sched_entity *se)
        {
            avg->load_avg += se->avg->load_avg;
            avg->load_sum += se->weight * se->avg->load_sum;
        }

        void enqueue_runnable_load_avg (sched_entity * se) {
            runnable_weight += se->runnable_weight;

            avg->runnable_load_avg += se->avg->runnable_load_avg;
            avg->runnable_load_sum += se->runnable_weight * se->avg->runnable_load_sum;
        }

        void account_entity_enqueue(sched_entity * se) {
            weight += se->weight;
            nr_running ++ ;
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

        void update_curr() {
            unsigned long now = jiffies_to_msecs(jiffies);
            unsigned long delta_exec;

            if (!curr) return;

            delta_exec = now - curr->exec_start;

            if (delta_exec <= 0) return;

            curr->exec_start = 0;
            curr->sum_exec_runtime += delta_exec;
            curr->vruntime += (delta_exec * NICE_0_LOAD / curr->weight);             // TODO: check the correctness

            

            /* TODO: update the min_vruntime */
            // update_min_vruntime(cfs_rq);


            // trace_sched_stat_runtime(curtask, delta_exec, curr->vruntime);
            // cgroup_account_cputime(curtask, delta_exec);
            // account_group_exec_runtime(curtask, delta_exec);

            // account_cfs_rq_runtime(cfs_rq, delta_exec);
        }

        /* insert the sched_entity into the rb_tree */
        void __enqueue_entity(sched_entity * se) {
            return;
        }
};

class rq {
    public:
        unsigned long   weight;
        int             overload;

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

        void activate_task(task * p, int flags) {
            /* TODO: update nr_uninterruptible if needed */
            enqueue_task(p, flags);
        }

        void enqueue_task(task * p, int flags) {
            cfs_runqueue->enqueue_entity(p->se, flags);
            /* TODO: check if se is a task 
             * If so, update the rq's weight
             */
            weight += p->se->weight;
            cfs_runqueue->h_nr_running++;
            add_nr_running(1);
        }

        private:
            void add_nr_running(unsigned int count) {
                unsigned int prev_nr = nr_running;
                nr_running = nr_running + count;
                if(prev_nr < 2 && nr_running >= 2) {
                    overload = true;
                }
            }

};


#endif
