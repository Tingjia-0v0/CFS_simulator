# include "sched.hpp"
# define DEBUG


int main(int argc, char *argv[])
{
    sched * global_sched = new sched();

    # ifdef DEBUG
        global_sched->debug_cputopo();
        global_sched->debug_sched(0);
    # endif

}
