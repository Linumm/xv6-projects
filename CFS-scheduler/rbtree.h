#ifndef RBTREE_H
# define RBTREE_H

// rbtree.h
# include "macro.h"

# define RB_RED		0
# define RB_BLACK	1

# define __rb_color(pc)		((pc) & 1)		// get the last color-bit
# define __rb_is_black(pc)	__rb_color(pc)	// if the last bit 1: black
# define __rb_is_red(pc)	(!__rb_color(pc))	// last bit 0: red

# define rb_color(rb)		__rb_color((rb)->__rb_parent_color)
# define rb_is_red(rb)		__rb_is_red((rb)->__rb_parent_color)
# define rb_is_black(rb)	__rb_is_black((rb)->__rb_parent_color)

# define rb_parent(r)		((struct rb_node *)((r)->__rb_parent_color & ~3))
# define __rb_parent(pc)	((struct rb_node *)(pc & ~3))

# define rb_entry(ptr, type, member) \
							container_of(ptr, type, member)

typedef unsigned long long u64;

struct rb_node
{
  // includes both parent address and this node's color
  // by using the last 2 bits as color bits (so that can save memory)
  unsigned long __rb_parent_color;
  struct rb_node *rb_right;
  struct rb_node *rb_left;

  u64 key;

} __attribute__((aligned(sizeof(long))));


struct rb_root
{
  struct rb_node *rb_node;
};


#endif
