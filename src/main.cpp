# include "sched.hpp"
# define DEBUG


int main(int argc, char *argv[])
{
    init_cpus();
    # ifdef DEBUG
        debug_cputopo();
    # endif
    max_load_balance_interval = HZ * num_online_cpus()/10;

    sched * global_sched = new sched();

    # ifdef DEBUG
        global_sched->debug_sched(0);
    # endif

}
