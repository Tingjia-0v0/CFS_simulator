#include "cpu.hpp"

std::vector<cputopo *> cpu_topology;
cpumask * cpu_online_mask = new cpumask();

int num_online_cpus() {
    cpumask::cpumask_weight(cpu_online_mask);
}

void init_cpus() {
    for (int i = 0; i < 64; i++) cpu_topology.push_back(NULL);
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
