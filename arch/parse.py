import json

def parse_cpu_range(cpus):
    cpu_masks = []
    cpu_ranges = cpus.split(',')
    for cpu_range in cpu_ranges:
        border = cpu_range.split('-')
        if len(border) == 1:
            cpu_masks.append(int(border[0]))
        elif len(border) == 2:
            for i in range(int(border[0]), int(border[1]) + 1):
                cpu_masks.append(i)
    return cpu_masks

with open('lscpu.json') as f:
    cpu_info = json.load(f)

online_cpus = cpu_info['online_cpus']

online_cpu_masks = parse_cpu_range(online_cpus)

numa_neighbor_mask = []
for node in cpu_info['nodes']:
    neighbor_cpus = []
    for neighbor_node in node['neighbor']:
        neighbor_cpus += parse_cpu_range(cpu_info['nodes'][neighbor_node]["cpus"])
    numa_neighbor_mask.append(neighbor_cpus)


cpu_topos = {}
for i, node in enumerate(cpu_info['nodes']):
    for core in node['cores']:
        for cpu in core['cpus']:
            thread_id = cpu
            core_id = core['id']
            socket_id = node['id']
            thread_sibling = core['cpus']
            core_sibling = parse_cpu_range(node["cpus"])
            numa_neighbor_sibling = numa_neighbor_mask[i]
            cpu_topos[thread_id] = {
                "thread_id": thread_id,
                "core_id": core_id,
                "socket_id": socket_id,
                "thread_sibling": thread_sibling,
                "core_sibling": core_sibling,
                "numa_neighbor_sibling": numa_neighbor_sibling
            }

print(online_cpu_masks)
print(cpu_topos)
results = {}
results["online_cpu_masks"] = online_cpu_masks
results["cpu_topots"] = cpu_topos
with open("lscpu_parsed.json", "w") as f:
    json.dump(results, f)

