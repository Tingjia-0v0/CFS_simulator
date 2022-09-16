#include "runqueue.hpp"

cfs_rq::cfs_rq(int _cpu) {
    // tasks_timeline = new rb_tree();
    weight = 0;
    runnable_weight = 0;
    nr_running = 0;
    exec_clock = 0;

    curr = next = last = skip = NULL;
    avg = new sched_avg();
    min_vruntime = 0;
    cpu = _cpu;

    tasks_timeline.rb_leftmost = NULL;
    tasks_timeline.rb_root.rb_node = NULL;
}

void cfs_rq::attach_entity_load_avg(sched_entity * se) {
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
void cfs_rq::update_load_avg(sched_entity * se, int flags) {

    unsigned long now = jiffies_to_msecs(jiffies) * 1000000;
    if (se->avg->last_update_time && !(flags & SKIP_AGE_LOAD))
        se->update_load_avg(now, cpu, curr == se);
    
    int decayed  = update_cfs_rq_load_avg(now);

    if (!se->avg->last_update_time && (flags & DO_ATTACH)) 
        attach_entity_load_avg(se);

}

void cfs_rq::enqueue_entity(sched_entity * se, int flags) {
    bool renorm = !(flags & ENQUEUE_WAKEUP) || (flags & ENQUEUE_MIGRATED);
    bool if_curr = curr == se;
    if (renorm && if_curr)
        se->update_vruntime(se->vruntime + min_vruntime);

    update_curr();

    if (renorm && !if_curr) 
        se->update_vruntime(se->vruntime + min_vruntime);

    update_load_avg(se, UPDATE_TG | DO_ATTACH);
    enqueue_runnable_load_avg(se);
    account_entity_enqueue(se);

    /*
        * TODO:
        * if (flags & ENQUEUE_WAKEUP)
        *     place_entity(cfs_rq, se, 0);
        */
    
    if (!if_curr)
        __enqueue_entity(se);

    se->on_rq = 1;
    debug_tasktimeline(&tasks_timeline);
}

void cfs_rq::dequeue_entity(sched_entity * se, int flags) {
    update_curr();
    update_load_avg(se, UPDATE_TG);
    dequeue_runnable_load_avg(se);

    clear_buddies(se);

    if (se != curr)
        __dequeue_entity(se);

    se->on_rq = 0;
    account_entity_dequeue(se);

    if (!(flags & DEQUEUE_SLEEP))
        se->vruntime -= min_vruntime;

    if ((flags & (DEQUEUE_SAVE | DEQUEUE_MOVE)) == DEQUEUE_SAVE)
        update_min_vruntime();
    debug_tasktimeline(&tasks_timeline);
}

void cfs_rq::update_curr() {
    std::cout << "    start updating current task's vruntime" << std::endl;
    unsigned long now = jiffies_to_msecs(jiffies) * 1000000;
    unsigned long delta_exec;

    if (!curr) return;

    delta_exec = now - curr->exec_start;

    if (delta_exec <= 0) return;

    curr->exec_start = now;
    curr->sum_exec_runtime += delta_exec;
    curr->update_vruntime(curr->vruntime +  (delta_exec * NICE_0_LOAD / curr->weight));  // TODO: check the correctness

    /* TODO: update the min_vruntime */
    update_min_vruntime();

    // trace_sched_stat_runtime(curtask, delta_exec, curr->0);
    // cgroup_account_cputime(curtask, delta_exec);
    // account_group_exec_runtime(curtask, delta_exec);
    // account_cfs_rq_runtime(cfs_rq, delta_exec);
}

void cfs_rq::set_next_buddy(struct sched_entity *se)
{
    next = se;
}

void cfs_rq::clear_buddies(sched_entity *se)
{
    if (last == se)
        last = NULL;

    if (next == se)
        next = NULL;

    if (skip == se)
        skip = NULL;
}

void cfs_rq::put_prev_entity(sched_entity * prev) {
    std::cout << "    put prev entity to cfs queue: " << prev->on_rq << std::endl;
    if (prev->on_rq) update_curr();
    if (prev->on_rq) {
        __enqueue_entity(prev);
        update_load_avg(prev, 0);
    }
    curr = NULL;
    debug_tasktimeline(&tasks_timeline);
}

sched_entity * cfs_rq::pick_next_entity(sched_entity *curr) {
    sched_entity * left = __pick_first_entity();
    sched_entity *se;
    if (!left || (curr && curr->vruntime < left->vruntime))
        left = curr;

    se = left;

    if (skip == se) {
        sched_entity *second;

        if (se == curr) {
            second = __pick_first_entity();
        } else {
            second = __pick_next_entity(se);
            if (!second || (curr && curr->vruntime < second->vruntime))
                second = curr;
        }

        if (second && second->wake_up_preempt_entity(left) < 1)
            se = second;
    }

    if (last && last->wake_up_preempt_entity(left) < 1)
        se = last;

    /*
    * Someone really wants this to run. If it's not unfair, run it.
    */
    if (next && next->wake_up_preempt_entity(left) < 1)
        se = next;

    clear_buddies(se);

    return se;

}

void cfs_rq::set_next_entity(sched_entity * se) {
    if (se->on_rq) {
        /*
        * Any task has to be enqueued before it get to execute on
        * a CPU. So account for the time it spent waiting on the
        * runqueue.
        */
        __dequeue_entity(se);
        update_load_avg(se, UPDATE_TG);
    }
    se->exec_start = jiffies_to_msecs(jiffies) * 100000;
    curr = se;

    se->prev_sum_exec_runtime = se->sum_exec_runtime;
    debug_tasktimeline(&tasks_timeline);
}

unsigned long cfs_rq::sched_slice(sched_entity * se) {
    unsigned long slice = __sched_period(nr_running + !se->on_rq);
    if (!se->on_rq) {
        weight += se->weight;
    }
    slice = __calc_delta(slice, se->weight, weight);

    return slice;
}

sched_entity * cfs_rq::__pick_first_entity() {
    rb_node *left = rb_first_cached(&tasks_timeline);
    if (!left) return NULL;
    return left->se;
}

int cfs_rq::update_cfs_rq_load_avg(unsigned long now) {
    int decayed = 0;
    decayed |= __update_load_avg_cfs_rq(now, cpu);
    // avg->debug_load_avg();
    return decayed;
}

        

unsigned int cfs_rq::__calc_delta(unsigned int delta_exec, unsigned long se_weight, unsigned int cfs_weight)
{
    return delta_exec * se_weight / cfs_weight;
}



unsigned long cfs_rq::__sched_period(unsigned long nr_running) {
    if (nr_running > sched_nr_latency)
        return nr_running * sysctl_sched_min_granularity;
    else
        return sysctl_sched_latency;
}

void cfs_rq::enqueue_load_avg(sched_entity *se)
{
    avg->load_avg += se->avg->load_avg;
    avg->load_sum += se->weight * se->avg->load_sum;
}

void cfs_rq::enqueue_runnable_load_avg (sched_entity * se) {
    // weight += se->weight;
    runnable_weight += se->runnable_weight;

    avg->runnable_load_avg += se->avg->runnable_load_avg;
    avg->runnable_load_sum += se->runnable_weight * se->avg->runnable_load_sum;
}

void cfs_rq::dequeue_runnable_load_avg (sched_entity * se) {
    runnable_weight -= se->runnable_weight;
    avg->runnable_load_avg -= se->avg->runnable_load_avg;
    avg->runnable_load_sum -= se->runnable_weight * se->avg->runnable_load_sum;
}

void cfs_rq::account_entity_enqueue(sched_entity * se) {
    weight += se->weight;
    nr_running ++ ;
    runqueue->add_to_cfs_tasks(se->container_task);
}

void cfs_rq::account_entity_dequeue(sched_entity * se) {
    weight -= se->weight;
    nr_running --;
    runqueue->erase_from_cfs_tasks(se->container_task);
}

int cfs_rq::__update_load_avg_cfs_rq(unsigned long now, int cpu)
{
    if (avg->_update_load_sum(now, cpu, weight, runnable_weight, curr!= NULL)) {
        avg->_update_load_avg(1, 1);
        return 1;
    }

    return 0;
}


void cfs_rq::update_min_vruntime() {
    unsigned long vruntime = min_vruntime;
    if (curr) {
        if (curr->on_rq) vruntime = curr->vruntime;
        else curr = NULL;
    }
    if (tasks_timeline.rb_leftmost) {
        if (!curr) 
            vruntime = tasks_timeline.rb_leftmost->vruntime;
        else if(vruntime > tasks_timeline.rb_leftmost->vruntime)
            vruntime = tasks_timeline.rb_leftmost->vruntime;
    }
    if (min_vruntime > vruntime)
        min_vruntime = min_vruntime;
    else
        min_vruntime = vruntime;

}

/* insert the sched_entity into the rb_tree */
void cfs_rq::__enqueue_entity(sched_entity * se) {
    struct rb_node ** link = &tasks_timeline.rb_root.rb_node;
    struct rb_node * parent = NULL;
    bool leftmost = true;

    while(*link) {
        parent = *link;
        if (se->vruntime < parent->vruntime)
            link = &parent->rb_left;
        else {
            link = &parent->rb_right;
            leftmost = false;
        }
    }
    rb_link_node(&se->run_node, parent, link);
    rb_insert_color_cached(&se->run_node,
                            &tasks_timeline, 
                            leftmost);
    return;
}

void cfs_rq::__dequeue_entity(sched_entity * se) {
    rb_erase_cached(&se->run_node, &tasks_timeline);
}


sched_entity * cfs_rq::__pick_next_entity(sched_entity * se) {
    rb_node *next = rb_next(&se->run_node);
    if (!next)
        return NULL;

    return next->se;
}


/* RQ functions */
rq::rq(int _cpu) {
    nr_running = 0;
    sd = NULL;
    cpu_capacity = 0;
    cpu = _cpu;

    sched_nr_latency = 8;

    cfs_runqueue = new cfs_rq(_cpu);
    cfs_runqueue->runqueue = this;

    idle = new task(cur_pid, new cpumask(), 0, 1);
    stop = new task(cur_pid, new cpumask(), 0, 1);
    curr = idle;
    idle->state = TASK_RUNNING;
    NEED_RESCHED_FLAG = 0;

    __preempt_count = 0;

    idle_balance = 1;

    next_balance = jiffies;
    max_idle_balance_cost = 0;
}

/*
    * util_avg = cfs_rq->util_avg / (cfs_rq->load_avg + 1) * se.load.weight
    *
    *  task  util_avg: 512, 256, 128,  64,  32,   16,    8, ...
    * cfs_rq util_avg: 512, 768, 896, 960, 992, 1008, 1016, ...
    * util_avg = running% * SCHED_CAPACITY_SCALE (1024)
    */
void rq::post_init_entity_util_avg(sched_entity * se) {
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

void rq::activate_task(task * p, int flags) {
    /* TODO: update nr_uninterruptible if needed */
    enqueue_task(p, flags);
}

void rq::deactivate_task(task * p, int flags) {
    /* TODO: update nr_uninterruptible if needed */
    dequeue_task(p, flags);
}

void rq::check_preempt_curr(task *p, int flags) {
    check_preempt_wakeup(p, flags);
}

void rq::check_preempt_tick(sched_entity * _curr) {
    unsigned long ideal_runtime, delta_exec;
    sched_entity * se;
    signed long delta;

    ideal_runtime = cfs_runqueue->sched_slice(_curr);

    /* delta_exec: exec time in this round */
    delta_exec = _curr->sum_exec_runtime - _curr->prev_sum_exec_runtime;

    if (delta_exec > ideal_runtime) {
        resched_curr();
        /*
        * The current task ran long enough, ensure it doesn't get
        * re-elected due to buddy favours.
        */
        cfs_runqueue->clear_buddies(_curr);
        return;
    }

    if (delta_exec < sysctl_sched_min_granularity)
        return;

    se = cfs_runqueue->__pick_first_entity();
    delta = _curr->vruntime - se->vruntime;

    if (delta < 0)
        return;

    if (delta > ideal_runtime)
        resched_curr();

}

void rq::schedule() {
    do {
        // preempt_disable();
        __schedule(false);
        // sched_preempt_enable_no_resched();
    } while(need_resched());
}

void rq::preempt_enable() {
    if (preempt_count_dec_and_test()) 
        preempt_schedule(); 
}

void rq::preempt_disable() {
    preempt_count_inc(); 
}

int rq::idle_cpu() {
    if (curr != idle)
        return 0;
    if (nr_running)
        return 0;
    return 1;
}

void rq::update_blocked_averages()
{
    unsigned long now = jiffies_to_msecs(jiffies) * 1000000;
    cfs_runqueue->update_cfs_rq_load_avg(now);
}

void rq::task_tick_fair(task *_curr, int queued) {
    entity_tick(curr->se, queued);
}

void rq::entity_tick(sched_entity * _curr, int queued) {
    cfs_runqueue->update_curr();
    cfs_runqueue->update_load_avg(_curr, UPDATE_TG);
    if (queued) {
        resched_curr();
        return;
    }

    if (cfs_runqueue->nr_running > 1) {
        check_preempt_tick(_curr);
    }
        
}    

void rq::preempt_count_dec() {
    preempt_count_sub(1);
}

int rq::preempt_count_dec_and_test() {
    preempt_count_sub(1);
    return should_resched(0);
}

void rq::preempt_count_inc() {
    preempt_count_add(1);
}

void rq::preempt_count_add(int val) {
    __preempt_count += val;
}

void rq::preempt_count_sub(int val) {
    if (val > __preempt_count) {
        std::cout << "preempt_count_sub error" << std::endl;
        return;
    }
        
    __preempt_count -= val;
}

void rq::preempt_schedule() {
    if(__preempt_count == 0 ) {
        // preempt current task
        do {
            preempt_disable();
            __schedule(true);
            preempt_count_dec();

        } while (need_resched());
    }
    return;
}

int rq::should_resched(int preempt_offset) {
    return __preempt_count == preempt_offset &&
            curr->test_tsk_need_resched();
}

void rq::add_nr_running(unsigned int count) {
    unsigned int prev_nr = nr_running;
    nr_running = nr_running + count;
    if(prev_nr < 2 && nr_running >= 2) {
        overload = true;
    }
}
void rq::sub_nr_running(unsigned int count) {
    nr_running = nr_running - count;
}

void rq::enqueue_task(task * p, int flags) {
    cfs_runqueue->enqueue_entity(p->se, flags);
    /* TODO: check if se is a task 
        * If so, update the rq's weight
        */
    weight += p->se->weight;
    cfs_runqueue->h_nr_running++;
    add_nr_running(1);
}

void rq::dequeue_task(task * p, int flags) {
    cfs_runqueue->dequeue_entity(p->se, flags);
    weight -= p->se->weight;
    cfs_runqueue->h_nr_running--;
    sub_nr_running(1);
}


void rq::check_preempt_wakeup(task * p, int wake_flags) {
    sched_entity * se = curr->se;
    sched_entity * pse = p->se;

    if (curr == idle) {
        std::cout << "    preempting current task" << std::endl;
        // cfs_runqueue->set_next_buddy(pse);
        resched_curr();
    }

    int scale = cfs_runqueue->nr_running >= sched_nr_latency; 

    int next_buddy_marked = 0;

    if (se == pse)
        return;

    /* RESCHED Flag has been setted before */
    if (curr->test_tsk_need_resched())
        return;
    
    /* TODO: find matching se */
    cfs_runqueue->update_curr();

    if (pse->wake_up_preempt_entity(se) == 1) {
        cfs_runqueue->set_next_buddy(pse);
        std::cout << "    preempting current task" << std::endl;
        resched_curr();

    }

}

void rq::resched_curr() {
    if (curr->test_tsk_need_resched())
        return;
    curr->set_tsk_need_resched();
    NEED_RESCHED_FLAG = 1;
}

int rq::need_resched() {
    return curr->test_tsk_need_resched();
}

void rq::__schedule(bool preempt) {
    task * prev, *next;
    unsigned long * switch_count;

    prev = curr;

    if (!preempt && prev->state) {
        if(prev->signal_pending_state(prev->state)) {
            prev->state = TASK_RUNNING;
        } else {
            deactivate_task(prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);
            prev->on_rq = 0; 
        }

    }
    next = pick_next_task(prev);
    std::cout << "    prev task: " << prev->pid << std::endl;
    std::cout << "    next task: " << next->pid << std::endl;
    prev->clear_tsk_need_resched();
    NEED_RESCHED_FLAG = 0;
    if (prev != next) {
        curr = next;
    }

}

task * rq::pick_next_task(task * prev) {
    task * p = pick_next_task_fair(prev);
    return p;
}

task * rq::pick_next_task_fair(task * prev) {
    sched_entity * se;
    task * p;
    int new_tasks;

    if (!nr_running) {
        /* TODO: do idle rebalance here */
        return idle;
    }

    put_prev_task(prev);
    se = cfs_runqueue->pick_next_entity(NULL);
    cfs_runqueue->set_next_entity(se);

    p = se->container_task;
    move_to_front_cfs_tasks(p);
    return p;
}

void rq::put_prev_task(task * prev) {
    put_prev_task_fair(prev);
}

void rq::put_prev_task_fair(task * prev) {
    cfs_runqueue->put_prev_entity(prev->se);
}

void rq::erase_from_cfs_tasks(task * p) {
    auto it = cfs_tasks.begin();
    for(; it != cfs_tasks.end(); it++) {
        if(*it == p ) {
            cfs_tasks.erase(it);
            break;
        }
    }
}

void rq::add_to_cfs_tasks(task *p) {
    cfs_tasks.insert(cfs_tasks.begin(), p);
}

void rq::move_to_front_cfs_tasks(task *p) {
    erase_from_cfs_tasks(p);
    add_to_cfs_tasks(p);
}