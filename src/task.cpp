# include "task.hpp"
sched_entity::sched_entity(int pid) {
    weight = 0;
    runnable_weight = 0;
    on_rq = 0;
    exec_start = 0;
    sum_exec_runtime = 0;
    vruntime = 0;
    prev_sum_exec_runtime = 0;
    nr_migrations = 0;

    avg = new sched_avg();

    run_node->vruntime = 0;
    run_node->color = RB_RED;
    run_node->rb_left = run_node->rb_right = run_node->rb_parent = NULL;
    run_node->se = this;
    run_node->pid = pid;
}

int sched_entity::update_load_avg(unsigned long now, int cpu, int running) {
    runnable_weight = weight;
    if (avg->_update_load_sum(now, cpu, !!on_rq, !!on_rq, running)) {
        avg->_update_load_avg(weight, runnable_weight);
        return 1;
    }

    return 0;
}

void sched_entity::update_vruntime(unsigned long new_vruntime){
    vruntime = new_vruntime;
    run_node->vruntime = new_vruntime;
}

int sched_entity::wake_up_preempt_entity(sched_entity * se) {
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

long sched_entity::wakeup_gran(sched_entity * se) {
    unsigned long gran = sysctl_sched_wakeup_granularity;
    return gran;
}
