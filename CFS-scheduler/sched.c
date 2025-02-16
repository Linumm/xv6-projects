#include "sched.h"


// rbtree.c
extern void rb_insert(struct rb_node*, struct rb_root*);
extern void rb_delete(struct rb_node*, struct rb_root*);
extern struct rb_node* rb_leftmost(struct rb_root*);


// console.c - to debug
extern void cprintf(char*, ...);


const int prio_to_weight[40] = {
  /* -20 */	88761,	71755,	56483,	46273,	36291,
  /* -15 */	29154,	23254,	18705,	14949,	11916,
  /* -10 */	 9548,	 7620,	 6100,	 4904,	 3906,
  /*  -5 */	 3121,	 2501,	 1991,	 1586,	 1277,
  /*   0 */	 1024,	  820,	  655,	  526,	  423,
  /*   5 */	  335,	  272,	  215,	  172,	  137,
  /*  10 */	  110,	   87,	   70,	   56,	   45,
  /*  15 */	   36,	   29,	   23,	   18,	   15,
};



/*
 * pre-defined inverse weight mult
 * wmult = 2^32 / weight
 */
const uint prio_to_wmult[40] = {
  /* -20 */	    48388,	    59856,		  76040,  		92818,	   118348,
  /* -15 */	   147320,	   184698,	   229616,	   287308,	   360437,
  /* -10 */	   449829,	   563644,	   704093,	   875809,	  1099582,
  /*  -5 */	  1376151,	  1717300,	  2157191,	  2708050,	  3363326,
  /*   0 */	  4194304,	  5237765,	  6557202,	  8165337,	 10153587,
  /*   5 */	 12820798,	 15790321,	 19976592,	 24970740,	 31350126,
  /*  10 */	 39045157,	 49367440,	 61356676,	 76695844,	 95443717,
  /*  15 */	119304647,	148102320,	186737708,	238609294,	286331153,
};


/* ----- Load Weight modificatino ----- */
static void
update_qinv_weight(struct load_weight *lw)
{
  uint weight = lw->weight;

  if (unlikely(!weight))
    lw->inv_weight = WMULT_CONST;
  else
    lw->inv_weight = WMULT_CONST / weight;
}


void
init_cfs_rq(struct cfs_rq* cfs_rq)
{
  cfs_rq->load.nice = 0;
  cfs_rq->load.weight = 0;
  cfs_rq->load.inv_weight = 0;

  cfs_rq->nr_running = 0;
  cfs_rq->min_vruntime = 0xFFFFFFFF; // 32-bit max uint
  
  cfs_rq->curr = 0;
}

/* ----- Runqueue modification ----- */
void
enqueue_entity_fair(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
  if(!se) return;
  if(se->on_rq) {
    //cprintf("ENQ Fail: Already in RQ!\n");
    return;
  }

  struct rb_root *root = &cfs_rq->proc_timeline;
  struct rb_node *node = &se->run_node;

  /* Updates key value right before insert into rbtree */
  node->key = se->vruntime;
  rb_insert(node, root);

  se->on_rq = 1;
  se->cfs_rq = cfs_rq;

  cfs_rq->nr_running++;
  cfs_rq->load.weight += se->load.weight;
  update_qinv_weight(&cfs_rq->load);
  cfs_rq->leftmost = rb_leftmost(root);
}


void
dequeue_entity_fair(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
  if(!se) return;
  if(!se->on_rq) {
    //cprintf("DEQ Fail: Already out of RQ!\n");
    return;
  }

  struct rb_root *root = &cfs_rq->proc_timeline;
  struct rb_node *node = &se->run_node;

  rb_delete(node, root);
  se->on_rq = 0;
  // keeping cfs_rq data for re-enqueue
  
  cfs_rq->nr_running--;
  cfs_rq->load.weight -= se->load.weight;
  update_qinv_weight(&cfs_rq->load);
  cfs_rq->leftmost = rb_leftmost(root);
}


struct sched_entity*
pick_entity_fair(struct cfs_rq *cfs_rq)
{
  if(!cfs_rq->nr_running || !cfs_rq->leftmost) 
	  return 0;
  
  struct sched_entity *se = 0;
  se = se_entry(cfs_rq->leftmost, struct sched_entity, run_node);

  return se;
}


/* ----- Calculating entity time slice data ----- */
u64
calc_period(uint nr_running)
{
  if(unlikely(nr_running > SCHED_NR_LATENCY))
    return nr_running * SCHED_MIN_GRANULARITY;
  return SCHED_LATENCY_US;
}


/* Calculate time slice */
u64
calc_delta(u64 delta_exec, uint weight, uint q_inv_weight)
{
  int shift = 32; // WMULT_SHIFT

  u64 mult = (u64)weight * (u64)q_inv_weight;
  u64 delta = delta_exec * mult;
  return (delta >> shift);
}


u64
calc_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
  u64 slice = calc_period(cfs_rq->nr_running + !se->on_rq);
  uint se_weight = se->load.weight;
  uint q_inv_weight = cfs_rq->load.inv_weight;

  slice = calc_delta(slice, se_weight, q_inv_weight);

  if(ALLOW_LOG)
    cprintf("\nw: %d, qinv: %d => slice: %d\n", se_weight, q_inv_weight, (uint)slice);
  return slice;
}


u64
calc_delta_vslice(u64 delta, struct sched_entity *se)
{
  delta *= (NICE_0_WEIGHT / se->load.weight);

  return delta;
}


/* ----- Updating proc's schedule stats in cfs_rq ----- */

/*
 * Update currently running proc's sched entity data
 * It is called from timer interrupt (every tick) with tick value
 */
void
update_entity_stat(struct cfs_rq *cfs_rq, u64 us)
{
  struct sched_entity *curr = cfs_rq->curr;
  if(unlikely(!curr)) return;

  u64 now = us;
  u64 delta_exec = now - curr->exec_start;

  curr->exec_start = now;
  //curr->prev_sum_exec_runtime = curr->sum_exec_runtime;
  curr->sum_exec_runtime += delta_exec;
  curr->tot_exec_runtime += delta_exec;
  curr->vruntime += calc_delta_vslice(delta_exec, curr);
  
  // update run_node's key - [NEED TO CHECK]
  struct rb_node *run_node = &curr->run_node;
  run_node->key = curr->vruntime;

  update_min_vruntime(cfs_rq);
}


/*
 * Update min_vruntime of cfs_rq, from current se and leftmost se
 * New min_vruntime is the min(curr, leftmost)
 * Since time passed, choose max(cfs_rq->min_vruntime, new min_vruntime)
 */
void
update_min_vruntime(struct cfs_rq *cfs_rq)
{
  struct sched_entity *curr = cfs_rq->curr;
  struct rb_node *leftmost = cfs_rq->leftmost;
  u64 vruntime = cfs_rq->min_vruntime;
  u64 new_min = vruntime;

  if(curr)
    new_min = curr->vruntime;
  if(leftmost) {
    struct sched_entity *se = rb_entry(leftmost, struct sched_entity, run_node);
    new_min = min(new_min, se->vruntime);
  }

  cfs_rq->min_vruntime = max(vruntime, new_min);

  return;
}


/* ----- Stage to judge/prepare for yield() ----- */
int
check_yield(struct cfs_rq *cfs_rq)
{
  /* If yield condition true ? 1 : 0 */

  struct sched_entity *curr = cfs_rq->curr;
  u64 runtime = curr->sum_exec_runtime;

  /* Ensure the min granularity */
  if(runtime < SCHED_MIN_GRANULARITY)
	  return 0;

  u64 ideal_runtime = calc_slice(cfs_rq, curr);

  if(ALLOW_LOG) {
    cprintf("runtime: %d, ideal: %d\n", (uint)runtime, (uint)ideal_runtime);
    cprintf("vruntime: %d, nr: %d\n", (uint)curr->vruntime, cfs_rq->nr_running);
  }

  /* if runtime over ideal runtime, yield */
  if(runtime >= ideal_runtime) {
    if(ALLOW_LOG) cprintf("[OVER IDEAL RUNTIME]\n");
	  return 1;
  }

  struct sched_entity *leftmost = pick_entity_fair(cfs_rq);
  signed long long delta_vruntime = (u64)curr->vruntime - (u64)leftmost->vruntime;

  /* if leftmost vruntime is smaller, yield */
  if(delta_vruntime > 0) {
    if(ALLOW_LOG) cprintf("[SMALLER VRUNTIME FOUND]: %d\n", (uint)leftmost->vruntime);
	  return 1;
  }

  //if(delta_vruntime > ideal_runtime)
	//return 1;
  return 0;
}


/* ----- sched_entity modification ----- */
void
init_entity(struct sched_entity *se)
{
  se->on_rq = 0;

  // Default nice : 20
  se->load.nice = 0;
  se->load.weight = prio_to_weight[0+20];
  se->load.inv_weight = prio_to_wmult[0+20];

  se->run_node.__rb_parent_color = 0;
  se->run_node.rb_right = 0;
  se->run_node.rb_left = 0;

  se->exec_start = 0;
  se->sum_exec_runtime = 0;
  se->vruntime = 0;
  se->tot_exec_runtime = 0;
}


void
copy_entity(struct sched_entity *pse, struct sched_entity *cse)
{
  struct cfs_rq *cfs_rq = pse->cfs_rq;
  cse->on_rq = 0;

  cse->load.nice = pse->load.nice;
  cse->load.weight = pse->load.weight;
  cse->load.inv_weight = pse->load.inv_weight;
  cse->cfs_rq = cfs_rq; // affinity?

  // Execute forked-child first
  cse->exec_start = 0;
  cse->sum_exec_runtime = 0;
  cse->vruntime = cfs_rq->min_vruntime;
  cse->tot_exec_runtime = 0;
}


void
reset_entity(struct sched_entity *se, u64 us)
{
  // set exec_start as current time
  // clear sum_exec_runtime of entity
  se->exec_start = us;
  se->sum_exec_runtime = 0;
}


int
get_nice_entity(struct sched_entity *se)
{
  return se->load.nice;
}


void
set_nice_entity(struct sched_entity *se, int nice)
{
  se->load.nice = nice;
  se->load.weight = prio_to_weight[nice+20];
  se->load.inv_weight = prio_to_wmult[nice+20];
}