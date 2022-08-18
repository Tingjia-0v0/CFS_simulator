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

std::vector<cputopo *> cpu_topology;
cpumask * cpu_online_mask;

int num_online_cpus() {
    cpumask::cpumask_weight(cpu_online_mask);
}

void init_cpus() {
    std::ifstream ifs("/users/Tingjia/simulator/arch/lscpu_parsed.json");
    json data = json::parse(ifs);
    const auto& arr = data["online_cpu_masks"]; 
    for(auto & i : arr){
        cpu_online_mask->set(i);
    }
    const auto & dict = data["cpu_topots"];
    for (auto& el : dict.items()) {
        cpu_topology[std::stoi(el.key())] = new cputopo(el.value()["thread_id"], el.value()["core_id"], el.value()["socket_id"]);
        for(auto & i: el.value()["thread_sibling"])
            cpu_topology[std::stoi(el.key())]->thread_sibling->set(i);
        for(auto & i: el.value()["core_sibling"])
            cpu_topology[std::stoi(el.key())]->core_sibling->set(i);
    }
}

void debug_cputopo() {
    std::cout << "cpu_online_mask: ";
    cpu_online_mask->debug_print_cpumask();
    std::cout << std::endl;
    for (auto & cpu : cpu_topology) {
        std::cout << cpu->thread_id << ":" << std::endl;
        std::cout << "\tcoreid: " << cpu->core_id << " thread_id: " << cpu->socket_id << std::endl;
        std::cout << "\tthread_sibling: ";
        cpu->thread_sibling->debug_print_cpumask();
        std::cout << "\tcore_sibling: ";
        cpu->core_sibling->debug_print_cpumask();
    }
    /* print cpu_online_mask and cpu_topology */
}


#endif