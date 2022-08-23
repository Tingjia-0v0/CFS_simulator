# ifndef UTIL_H
# define UTIL_H

# define ULONG_MAX  (~0UL)
# define LONG_MAX   ((long)(~0UL>>1))
# define MSEC_PER_SEC       1000
# define HZ                 250

# define for_each_cpu(cpu, mask)	\
    for (int _first = 1, cpu = mask->first(); \
         (cpu != mask->first() || _first) && cpu != -1; \
         cpu = mask->next(cpu), _first = 0 )

# define for_each_cpu_from(cpu, mask, offset) \
    for (int _first = 1, cpu = mask->next(offset); \
         (cpu != mask->next(offset) || _first) && cpu != -1; \
         cpu = mask->next(cpu), _first = 0 )
# endif
