#include "rbtree.h"
#include "types.h"



static inline void
rb_set_black(struct rb_node *rb)
{
  rb->__rb_parent_color += RB_BLACK;
}


static inline struct rb_node
*rb_red_parent(struct rb_node *red)
{
  return (struct rb_node *)red->__rb_parent_color;
}


/*
 * change child: change child of parent from old to new
 * if parent is null, it becomes root
 */
static inline void
__rb_change_child(struct rb_node *old, struct rb_node *new,
				  struct rb_node *parent, struct rb_root *root)
{
  if(parent) {
	if(parent->rb_left == old)
	  parent->rb_left = new;
	else
	  parent->rb_right = new;
  }
  else	// if parent is null, new is root node
	root->rb_node = new;
}


static inline void
rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
  rb->__rb_parent_color = rb_color(rb) + (unsigned long)p;
}


static inline void
rb_set_parent_color(struct rb_node *node, struct rb_node *p, int color)
{
  node->__rb_parent_color = (unsigned long)p + color;
}


static inline void
__rb_rotate_set_parents(struct rb_node *old, struct rb_node *new,
						struct rb_root *root, int color)
{
  struct rb_node *p = rb_parent(old);
  new->__rb_parent_color = old->__rb_parent_color;
  rb_set_parent_color(old, new, color);
  __rb_change_child(old, new, p, root);
}


// fixing after insertion by rb-tree condition
static void
rb_insert_fix(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *p = rb_red_parent(node);
  struct rb_node *gp, *tmp;


  while(1) {
	/* Loop invariant: node is RB_RED */
	if(!p) {
	  rb_set_parent_color(node, 0, RB_BLACK);
	  break;
	}
	
	/*
	 * If there's a RB_BLACK parent, end
	 * else, fix by condition: no-consecutive red nodes
	 */
	if(rb_is_black(p)) break;

	gp = rb_red_parent(p);
	tmp = gp->rb_right;

	if(p != tmp) { // p == gp->rb_left
	  if(tmp && rb_is_red(tmp)) {
		/* case1, p: red && u: red
		 *		GP				gp
		 *	   /  \			   /  \
		 *    p    u	=>	  P    U
		 *   /				 /
		 *  n				n
		 */

		rb_set_parent_color(tmp, gp, RB_BLACK);
		rb_set_parent_color(p, gp, RB_BLACK);
		node = gp;
		p = rb_parent(node);
		rb_set_parent_color(node, p, RB_RED);
		continue;
	  }

	  tmp = p->rb_right;
	  if(node == tmp) {
		/* case2, p: red && u: black && node is right of p
		 *		GP				GP
		 *	   /  \			   /  \
		 *	  p    U	=>	  n	   U
		 *	   \			 /
		 *		n			p
		 */

		tmp = node->rb_left;
		// Should block compiler optimization
		p->rb_right = tmp;
		node->rb_left = p;

		if(tmp) rb_set_parent_color(tmp, p, RB_BLACK);
		
		rb_set_parent_color(p, node, RB_RED);
		/* dummy rotate? */
		//rb_set_parent_color(node, gp, RB_RED);
		p = node;
		tmp = node->rb_right;
	  }
	  
	  /* case3, p: red && u: black && node is left of p
	   *		GP				P
	   *	   /  \			   / \
	   *	  p    U	=>	  n	 gp 
	   *	 /					   \
	   *	n						U
	   */
	  // Should block compiler optimization
	  gp->rb_left = tmp;
	  p->rb_right = gp;
	  if(tmp) rb_set_parent_color(tmp, gp, RB_BLACK);
	  __rb_rotate_set_parents(gp, p, root, RB_RED);
	  /* dummy rotate? */
	  break;
	}

	else { // p == gp->rb_right
	  tmp = gp->rb_left;
	  if(tmp && rb_is_red(tmp)) {
		/* flipped case of case1 */
		rb_set_parent_color(tmp, gp, RB_BLACK);
		rb_set_parent_color(p, gp, RB_BLACK);
		node = gp;
		p = rb_parent(node);
		rb_set_parent_color(node, p, RB_RED);
		continue;
	  }

	  tmp = p->rb_left;
	  if(node == tmp) {
		/* flipped case of case2 */
		tmp = node->rb_right;
		// Should block compiler optimization
		p->rb_left = tmp;
		node->rb_right = p;
		if(tmp) rb_set_parent_color(p, node, RB_RED);
		/* dummy rotate? */
		p = node;
		tmp = node->rb_left;
	  }

	  /* flipped case of case3 */
	  // Should block compiler optimization
	  gp->rb_right = tmp;
	  p->rb_left = gp;
	  if(tmp) rb_set_parent_color(tmp, gp, RB_BLACK);
	  __rb_rotate_set_parents(gp, p, root, RB_RED);
	  /* dummy rotate? */
	  break;
	}
  }
}


/*
 * node insertion
 */
static void
_rb_insert(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *tmp = root->rb_node;
  if(!tmp) {
	node->rb_left = node->rb_right = 0;
	rb_set_parent_color(node, 0, RB_BLACK);
	root->rb_node = node;
	return;
  }

  struct rb_node *p;
  uint key = node->key;

  while(tmp) {
	p = tmp;
	if(key < p->key)
	  tmp = p->rb_left;
	else
	  tmp = p->rb_right;
  } // now p is the leaf

  if(key < p->key)
	p->rb_left = node;
  else
	p->rb_right = node;

  rb_set_parent_color(node, p, RB_RED);
  node->rb_left = node->rb_right = 0;

  rb_insert_fix(node, root);
}


/*
 * successor finding in deletion process
 * default: leftmost successor (smallest bigger)
 */
static struct rb_node*
rb_successor(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *child = node->rb_right;
  struct rb_node *tmp = node->rb_left;
  struct rb_node *p, *rebalance;
  unsigned long pc;

  if(!tmp) {
	/*
	 * Case 1: node to erase has no more than 1 child
	 */
	pc = node->__rb_parent_color;
	p = __rb_parent(pc);
	__rb_change_child(node, child, p, root);
	if(child) {
	  child->__rb_parent_color = pc;
	  rebalance = 0;
	}
	else
	  // case-> the node was leaf
	  // If deleted node is black, need rebalance
	  rebalance = __rb_is_black(pc) ? p : 0;
	tmp = p;
  }

  else if(!child) {
	/*
	 * Still case 1, but this time, the child is node->rb_left
	 */
	tmp->__rb_parent_color = pc = node->__rb_parent_color;
	p = __rb_parent(pc);
	__rb_change_child(node, tmp, p, root);
	rebalance = 0;
	tmp = p;
  }

  else {
	struct rb_node *successor = child;
	struct rb_node *child2;

	tmp = child->rb_left;
	if(!tmp) {
	  /*
	   * Case 2: node's successor is its right child
	   *	n				s
	   *   / \			   / \
	   *  x   s		=>	  x   c
	   *	   \
	   *		c
	   */
	  p = successor;
	  child2 = successor->rb_right;
	  /* more? */
	}

	else {
	  /*
	   * Case 3: node's successor is leftmost under right child subtree
	   *	n				s
	   *   / \			   / \
	   *  x   y		=>	  x   y
	   *	 /				 /
	   *	p				p
	   *   /			   /
	   *  s				  c
	   *   \
	   *    c
	   */
	  do {
		p = successor;
		successor = tmp;
		tmp = tmp->rb_left;
	  } while(tmp);
	  child2 = successor->rb_right;	// c
	  // Should block compiler optimization
	  p->rb_left = child2;			// c
	  successor->rb_right = child;	// y
	  rb_set_parent(child, successor);
	}

	tmp = node->rb_left;			// x
	successor->rb_left = tmp;		// link (x,s)
	rb_set_parent(tmp, successor);

	pc = node->__rb_parent_color;
	tmp = __rb_parent(pc);			// link (s, node's origin p)
	__rb_change_child(node, successor, tmp, root);

	if(child2) {
	  // Since it is trivial that one of c2 & p is RB_BLACK
	  rb_set_parent_color(child2, p, RB_BLACK);
	  rebalance = 0;
	}
	else
	  // If s was RB_BLACK, then need to rebalance
	  rebalance = rb_is_black(successor) ? p : 0;
	
	successor->__rb_parent_color = pc;
	tmp = successor;
  }

  return rebalance;
}



// tree rebalancing after replacing with successor
static void
rb_rebalance(struct rb_node *p, struct rb_root *root)
{
  struct rb_node *node = 0;
  struct rb_node *sibling, *tmp1, *tmp2;

  while(1) {
	/*
	 * Loop invariants:
	 * - node is RB_BLACK (or NULL)
	 * - node is not the root
	 * - all leaf paths going through parent and node have a
	 *   black node count that is 1 lower than other leaf paths.
	 */
	sibling = p->rb_right;
	if(node != sibling) {	// node: p->rb_left
	  if(rb_is_red(sibling)) {
		/*
		 * Case 1: left rotate at p
		 *		P				S
		 *	   / \			   / \
		 *	  N   s		=>	  p  Sr
		 *		 / \		 / \
		 *		Sl  Sr		N  Sl
		 */
		tmp1 = sibling->rb_left;
		// Should block compiler optimization
		p->rb_right = tmp1;
		sibling->rb_left = p;
		rb_set_parent_color(tmp1, p, RB_BLACK);
		__rb_rotate_set_parents(p, sibling, root, RB_RED);
		sibling = tmp1;
	  }

	  tmp1 = sibling->rb_right;
	  if(!tmp1 || rb_is_black(tmp1)) {
		tmp2 = sibling->rb_left;
		if(!tmp2 || rb_is_black(tmp2)) {
		  /*
		   * Case 2: sibling color flip
		   * (p could be either color here)
		   *		(p)				(p)
		   *	    / \			    / \
		   *	   N   S	=>	   N   s
		   *		  / \			  / \
		   *		 Sl Sr			 Sl Sr
		   */
		  rb_set_parent_color(sibling, p, RB_RED);
		  if(rb_is_red(p))
			rb_set_black(p);
		  else {
			node = p;
			p = rb_parent(node);
			if(p) continue;
		  }
		  break;
		}

		/*
		 * Case 3: right rotate at sibling
		 *		(p)				(p)
		 *	    / \				/ \
		 *	   N   S	=>	   N  sl
		 *		  / \			   \
		 *		 sl sr			    S
		 *							 \
		 *							 sr
		 * if p is red, pass to case 4
		 */
		tmp1 = tmp2->rb_right;
		// Should block compiler optimization
		sibling->rb_left = tmp1;
		tmp2->rb_right = sibling;
		p->rb_right = tmp2;
		if(tmp1)
		  rb_set_parent_color(tmp1, sibling, RB_BLACK);
		tmp1 = sibling;
		sibling = tmp2;

	  }
	  
	  /*
	   * Case 4: left rotate at p + color flips
	   * (p and sl could be either color here.
	   *  After rotation, p becomes black, s acquires
	   *  p's color, and s1 keeps its color)
	   *
	   *		(p)				(s)
	   *		/ \				/ \
	   *	   N   S	=>	   P  SR
	   *		  / \		  / \
	   *		(sl) sr		 N (sl)
	   */
	  tmp2 = sibling->rb_left;
	  // Should block compiler optimization
	  p->rb_right = tmp2;
	  sibling->rb_left = p;
	  rb_set_parent_color(tmp1, sibling, RB_BLACK);
	  if(tmp2)
		rb_set_parent(tmp2, p);
	  __rb_rotate_set_parents(p, sibling, root, RB_BLACK);
	  break;
	}
	
	else { // direction flipped case
	  sibling = p->rb_left;
	  if(rb_is_red(sibling)) {
		/* flipped case 1 */
		tmp1 = sibling->rb_right;
		p->rb_left = tmp1;
		sibling->rb_right = p;
		rb_set_parent_color(tmp1, p, RB_BLACK);
		__rb_rotate_set_parents(p, sibling, root, RB_RED);
		sibling = tmp1;
	  }
	  
	  tmp1 = sibling->rb_left;
	  if(!tmp1 || rb_is_black(tmp1)) {
		tmp2 = sibling->rb_right;
		if(!tmp2 || rb_is_black(tmp2)) {
		  /* flipped case 2 */
		  rb_set_parent_color(sibling, p, RB_RED);
		  if(rb_is_red(p))
			rb_set_black(p);
		  else {
			node = p;
			p = rb_parent(node);
			if(p) continue;
		  }
		  break;
		}
		/* flipped case 3 */
		tmp1 = tmp2->rb_left;
		// Shoud block compiler optimization
		tmp1 = tmp2->rb_left;
		sibling->rb_right = tmp1;
		tmp2->rb_left = sibling;
		p->rb_left = tmp2;
		if(tmp1)
		  rb_set_parent_color(tmp1, sibling, RB_BLACK);
		tmp1 = sibling;
		sibling = tmp2;
	  }
	  /* flipped case 4 */
	  tmp2 = sibling->rb_right;
	  p->rb_left = tmp2;
	  sibling->rb_right = p;
	  rb_set_parent_color(tmp1, sibling, RB_BLACK);
	  if(tmp2)
		rb_set_parent(tmp2, p);
	  __rb_rotate_set_parents(p, sibling, root, RB_BLACK);
	  break;
	}
  }
}



// node deletion
static void
_rb_delete(struct rb_node *node, struct rb_root *root)
{
  struct rb_node *rebalance;
  rebalance = rb_successor(node, root);
  if(rebalance)
	rb_rebalance(rebalance, root);
}


/* ----- */
void
rb_insert(struct rb_node *node, struct rb_root *root)
{
  _rb_insert(node, root);
}

void
rb_delete(struct rb_node *node, struct rb_root *root)
{
  _rb_delete(node, root);
}

struct rb_node*
rb_leftmost(struct rb_root *root)
{
  struct rb_node *node = root->rb_node;
  if(!node) return 0;

  while(node->rb_left)
	node = node->rb_left;

  return node;
}
