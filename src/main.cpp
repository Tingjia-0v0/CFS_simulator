# include "sched.hpp"
# define DEBUG


int main(int argc, char *argv[])
{
    sched * global_sched = new sched("/users/Tingjia/CFS_simulator/arch/lscpu_parsed.json");

    # ifdef DEBUG
        global_sched->debug_sched(0);
    # endif

}
