# include "sched.hpp"
# define DEBUG

int cur_pid = 0;

int start_new_task(sched * global_sched, int cur_cpu, cpumask * cpu_allowed, int nice) {
    task * t = new task(cur_pid, cpu_allowed, nice);
    global_sched->wake_up_new_task(t, cur_cpu);
}

int main(int argc, char *argv[])
{
    sched * global_sched = new sched("/users/Tingjia/CFS_simulator/arch/lscpu_parsed.json");

    # ifdef DEBUG
        global_sched->debug_sched(0);
    # endif
    cpumask * cpu_allowed = new cpumask();
    for(int i = 0; i < NR_CPU; i++) cpu_allowed->set(i);

    jiffies = 1000;
    start_new_task(global_sched, 5, cpu_allowed, 0);

}
