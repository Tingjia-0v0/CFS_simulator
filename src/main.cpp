# include "sched.hpp"


int main(int argc, char *argv[])
{
    init_cpus();
    debug_cputopo();
    max_load_balance_interval = HZ * num_online_cpus()/10;

    sched * global_sched = new sched();

}
