#ifndef _RUNQUEUE_H
#define _RUNQUEUE_H 

# include "sched_domain.hpp"
# include "task.hpp"
# include "util.hpp"
# include "load.hpp"

extern int cur_pid;

class rq;
class cfs_rq {
    public:
        unsigned long weight;
        unsigned long runnable_weight;
        unsigned int  nr_running;
        unsigned int  h_nr_running;

        unsigned long exec_clock;
        unsigned long min_vruntime;

        sched_entity    *curr, *next, *last, *skip;

        sched_avg * avg;

        int             cpu;

        struct rb_root_cached tasks_timeline;

        int sched_nr_latency = 8;
        unsigned int sysctl_sched_min_granularity		= 750000ULL;
        unsigned int sysctl_sched_latency			= 6000000ULL;

        rq * runqueue;

        cfs_rq(int _cpu);

        void attach_entity_load_avg(sched_entity * se) ;
        /* update task and cfs's load_avg 
           TODO: now it only updates the cfs_rq's load_avg */
        void update_load_avg(sched_entity * se, int flags);
        void enqueue_entity(sched_entity * se, int flags);
        void dequeue_entity(sched_entity * se, int flags);
        void update_curr();
        void set_next_buddy(struct sched_entity *se);

        void clear_buddies(sched_entity *se);
        void put_prev_entity(sched_entity * prev);
        sched_entity * pick_next_entity(sched_entity *curr);
        void set_next_entity(sched_entity * se);
        unsigned long sched_slice(sched_entity * se);
        sched_entity * __pick_first_entity();

        int update_cfs_rq_load_avg(unsigned long now);

            
    private:

        

        unsigned int __calc_delta(unsigned int delta_exec, unsigned long se_weight, unsigned int cfs_weight);



        unsigned long __sched_period(unsigned long nr_running);

        void enqueue_load_avg(sched_entity *se);

        void enqueue_runnable_load_avg (sched_entity * se);

        void dequeue_runnable_load_avg (sched_entity * se);

        void account_entity_enqueue(sched_entity * se);

        void account_entity_dequeue(sched_entity * se);

        int __update_load_avg_cfs_rq(unsigned long now, int cpu);


        void update_min_vruntime();

        /* insert the sched_entity into the rb_tree */
        void __enqueue_entity(sched_entity * se);

        void __dequeue_entity(sched_entity * se);
        
        sched_entity * __pick_next_entity(sched_entity * se);
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

        unsigned char   idle_balance;

        task * curr, * idle, * stop;

        unsigned long   next_balance;

        unsigned long max_idle_balance_cost;

        int __preempt_count;

        int NEED_RESCHED_FLAG;

        int sched_nr_latency = 8;
        unsigned int sysctl_sched_min_granularity		= 750000ULL;
        unsigned int sysctl_sched_latency			= 6000000ULL;
        unsigned int sysctl_sched_migration_cost	= 500000UL;

        std::vector<task * > cfs_tasks;

        rq(int _cpu);

        /*
         * util_avg = cfs_rq->util_avg / (cfs_rq->load_avg + 1) * se.load.weight
         *
         *  task  util_avg: 512, 256, 128,  64,  32,   16,    8, ...
         * cfs_rq util_avg: 512, 768, 896, 960, 992, 1008, 1016, ...
         * util_avg = running% * SCHED_CAPACITY_SCALE (1024)
         */
        void post_init_entity_util_avg(sched_entity * se);

    
        void activate_task(task * p, int flags);
        void deactivate_task(task * p, int flags);

        void check_preempt_curr(task *p, int flags);

        void check_preempt_tick(sched_entity * _curr);
        void schedule();
        
        void preempt_enable();

        void preempt_disable();

        int idle_cpu();
        
        void update_blocked_averages();

        void task_tick_fair(task *_curr, int queued);

        void entity_tick(sched_entity * _curr, int queued);

        void preempt_count_dec();
        
        int preempt_count_dec_and_test();

        void preempt_count_inc();

        void preempt_count_add(int val);

        void preempt_count_sub(int val);

        void preempt_schedule();

        int should_resched(int preempt_offset);

        void add_nr_running(unsigned int count);

        void sub_nr_running(unsigned int count);
        
        void enqueue_task(task * p, int flags);

        void dequeue_task(task * p, int flags);


        void check_preempt_wakeup(task * p, int wake_flags);

        void resched_curr();

        int need_resched();

        void __schedule(bool preempt);

        task * pick_next_task(task * prev);

        task * pick_next_task_fair(task * prev);

        void put_prev_task(task * prev);

        void put_prev_task_fair(task * prev);

        void erase_from_cfs_tasks(task * p);

        void add_to_cfs_tasks(task *p);

        void move_to_front_cfs_tasks(task *p);

        unsigned long cpu_avg_load_per_task();
        

};


#endif
