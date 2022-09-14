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
    for (_sd = runqueues[cpu]->sd; _sd; _sd = _sd->parent)

# define for_each_domain_rq(_sd)   \
    for (_sd = this->sd; _sd; _sd = _sd->parent)

#define LOAD_AVG_PERIOD 32
#define LOAD_AVG_MAX 47742

/*
 * Optional action to be done while updating the load average
 */
#define UPDATE_TG	0x1
#define SKIP_AGE_LOAD	0x2
#define DO_ATTACH	0x4

#define DEQUEUE_SLEEP		0x01
#define DEQUEUE_SAVE		0x02 /* matches ENQUEUE_RESTORE */
#define DEQUEUE_MOVE		0x04 /* matches ENQUEUE_MOVE */
#define DEQUEUE_NOCLOCK		0x08 /* matches ENQUEUE_NOCLOCK */


#define ENQUEUE_WAKEUP		0x01
#define ENQUEUE_RESTORE		0x02
#define ENQUEUE_MOVE		0x04
#define ENQUEUE_NOCLOCK		0x08

#define ENQUEUE_HEAD		0x10
#define ENQUEUE_REPLENISH	0x20
#define ENQUEUE_MIGRATED	0x40

#define	RB_RED		0
#define	RB_BLACK	1

#define TIF_SYSCALL_TRACE	0	/* syscall trace active */
#define TIF_NOTIFY_RESUME	1	/* callback before returning to user */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
#define TIF_SINGLESTEP		4	/* reenable singlestep on user return*/
#define TIF_SYSCALL_EMU		6	/* syscall emulation active */
#define TIF_SYSCALL_AUDIT	7	/* syscall auditing active */
#define TIF_SECCOMP		8	/* secure computing */
#define TIF_USER_RETURN_NOTIFY	11	/* notify kernel of userspace return */
#define TIF_UPROBE		12	/* breakpointed or singlestepping */
#define TIF_PATCH_PENDING	13	/* pending live patching update */
#define TIF_NOCPUID		15	/* CPUID is not accessible in userland */
#define TIF_NOTSC		16	/* TSC is not accessible in userland */
#define TIF_IA32		17	/* IA32 compatibility process */
#define TIF_NOHZ		19	/* in adaptive nohz mode */
#define TIF_MEMDIE		20	/* is terminating due to OOM killer */
#define TIF_POLLING_NRFLAG	21	/* idle is polling for TIF_NEED_RESCHED */
#define TIF_IO_BITMAP		22	/* uses I/O bitmap */
#define TIF_FORCED_TF		24	/* true if TF in eflags artificially */
#define TIF_BLOCKSTEP		25	/* set when we want DEBUGCTLMSR_BTF */
#define TIF_LAZY_MMU_UPDATES	27	/* task is updating the mmu lazily */
#define TIF_SYSCALL_TRACEPOINT	28	/* syscall tracepoint instrumentation */
#define TIF_ADDR32		29	/* 32-bit address space on 64 bits */
#define TIF_X32			30	/* 32-bit native x86-64 binary */
#define TIF_FSCHECK		31	/* Check FS is USER_DS on return */

#define PREEMPT_NEED_RESCHED 0x80000000 

/*
 * wake flags
 */
#define WF_SYNC		0x01		/* waker goes to sleep after wakeup */
#define WF_FORK		0x02		/* child wakeup after fork */
#define WF_MIGRATED	0x4		/* internal use, task got migrated */

#define CPU_IDLE            0
#define	CPU_NOT_IDLE        1
#define	CPU_NEWLY_IDLE      2
#define	CPU_MAX_IDLE_TYPES  3

#define sysctl_sched_wakeup_granularity 1000000UL


#define regular         0
#define remote          1
#define all             2

#define LBF_ALL_PINNED	0x01
#define LBF_NEED_BREAK	0x02
#define LBF_DST_PINNED  0x04
#define LBF_SOME_PINNED	0x08

#define MAX_PINNED_INTERVAL	512

#define group_other         0
#define	group_imbalanced    1
#define	group_overloaded    2
# endif
