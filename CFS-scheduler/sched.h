#ifndef SCHED_H
#define SCHED_H

// sched.h
# include "rbtree.h"
# include "types.h"

/* In standard xv6, 1 tick = 10 ms
 * But I tuned 1 tick = 1 ms = 1000 us
 * And treat all data about time slice as 'us' unit
 * so, every tick, delta_exec goes up * 1000
 *
 * Set schedule latency	   as 18,000 us (18 ticks)
 * Set minimum granularity as  3,000 us ( 3 ticks)
 */

# define SCHED_LATENCY_US		18000
# define SCHED_MIN_GRANULARITY	 3000
# define SCHED_NR_LATENCY		(SCHED_LATENCY_US/SCHED_MIN_GRANULARITY)
# define NICE_0_WEIGHT			1024
# define WMULT_CONST        0xFFFFFFFF
# define ALLOW_LOG          0

# define se_entry(ptr, type, member) \
  				container_of(ptr, type, member)



extern const int prio_to_weight[40];
extern const uint prio_to_wmult[40];

struct load_weight
{
  int nice;
  uint weight;
  uint inv_weight;
};


struct cfs_rq
{
  struct load_weight	load;
  int                 nr_running;
  u64                 min_vruntime;

  struct rb_root		  proc_timeline;
  struct rb_node		  *leftmost;
  struct sched_entity	*curr;
};


struct sched_entity
{
  struct load_weight	load;
  struct rb_node      run_node;

  u64					exec_start;
  u64					sum_exec_runtime;
  u64					tot_exec_runtime;
  u64					vruntime;

  uint              on_rq;
  struct cfs_rq	    *cfs_rq;
};



/* ----- sched.c function declaration ----- */
u64 	calc_period(uint);
u64		calc_delta(u64, uint, uint);
u64		calc_slice(struct cfs_rq*, struct sched_entity*);
u64		calc_delta_vslice(u64, struct sched_entity*);

void 	update_entity_stat(struct cfs_rq*, u64);
void 	update_min_vruntime(struct cfs_rq*);
int 	check_yield(struct cfs_rq*);

void  clear_entity_stat(struct sched_entity*, u64);

#endif /* sched.h */
