# ifndef UTIL_H
# define UTIL_H

# define ULONG_MAX  (~0UL)
# define LONG_MAX   ((long)(~0UL>>1))
# define MSEC_PER_SEC       1000
# define HZ                 250
# define NR_CPU             64

# define NICE_0_LOAD_SHIFT	(10)
#define NICE_0_LOAD		(1L << NICE_0_LOAD_SHIFT)


/* task state bitmask, copied from include/linux/sched.h */
#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define __TASK_STOPPED		4
#define __TASK_TRACED		8
/* in tsk->exit_state */
#define EXIT_DEAD		16
#define EXIT_ZOMBIE		32
#define EXIT_TRACE		(EXIT_ZOMBIE | EXIT_DEAD)
/* in tsk->state again */
#define TASK_DEAD		64
#define TASK_WAKEKILL		128
#define TASK_WAKING		256
#define TASK_PARKED		512
#define TASK_NOLOAD			0x0400
#define TASK_NEW			0x0800
#define TASK_STATE_MAX			0x1000

#define MAX_NICE	19
#define MIN_NICE	-20
#define NICE_WIDTH	(MAX_NICE - MIN_NICE + 1)

#define MAX_USER_RT_PRIO	100
#define MAX_RT_PRIO		MAX_USER_RT_PRIO

#define DEFAULT_PRIO		(MAX_RT_PRIO + NICE_WIDTH / 2) // 100 + 40/2 = 120
#define NICE_TO_PRIO(nice)	((nice) + DEFAULT_PRIO)

# define for_each_cpu(cpu, mask)	\
    for (int _first = 1, cpu = mask->first(); \
         (cpu != mask->first() || _first) && cpu != -1; \
         cpu = mask->next(cpu), _first = 0 )

# define for_each_cpu_from(cpu, mask, offset) \
    for (int _first = 1, cpu = mask->next(offset); \
         (cpu != mask->next(offset) || _first) && cpu != -1; \
         cpu = mask->next(cpu), _first = 0 )

# define for_each_domain(_sd, cpu)   \
    for (_sd = runqueues[cpu]->sd; _sd; _sd = tmp->parent)

#define LOAD_AVG_PERIOD 32
#define LOAD_AVG_MAX 47742

/*
 * Optional action to be done while updating the load average
 */
#define UPDATE_TG	0x1
#define SKIP_AGE_LOAD	0x2
#define DO_ATTACH	0x4


# endif
