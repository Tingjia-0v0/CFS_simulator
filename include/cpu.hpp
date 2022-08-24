#ifndef _CPU_H
#define _CPU_H

# include <fstream>
# include <vector>
# include "cpumask.hpp"
# include <nlohmann/json.hpp>


using json = nlohmann::json;

class cputopo {
    public:
        int thread_id;
        int core_id;
        int socket_id;
        cpumask * thread_sibling;
        cpumask * core_sibling;
    public:
        const cpumask * get_smt_mask() {
            return thread_sibling;
        }
        const cpumask * get_coregroup_mask() {
            return core_sibling;
        }
        cputopo(int _thread_id, int _core_id, int _socket_id) {
            thread_id = _thread_id;
            core_id = _core_id;
            socket_id = _socket_id;
            thread_sibling = new cpumask();
            core_sibling = new cpumask();
        }
};

#endif