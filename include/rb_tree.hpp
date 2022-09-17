#ifndef	_RBTREE_H
#define	_RBTREE_H

#include <iostream>
#include "util.hpp"
#include "task.hpp"

class sched_entity;

struct rb_node {
    short color;
    struct rb_node *rb_parent;
	struct rb_node *rb_right;
	struct rb_node *rb_left;
    unsigned long  vruntime;
	int pid;
	sched_entity * se;
};

struct rb_root {
	struct rb_node *rb_node;
};

struct rb_root_cached {
	struct rb_root rb_root;
	struct rb_node *rb_leftmost;
};

struct rb_augment_callbacks {
	void (*propagate)(struct rb_node *node, struct rb_node *stop);
	void (*copy)(struct rb_node *old, struct rb_node *_new);
	void (*rotate)(struct rb_node *old, struct rb_node *_new);
};

void dummy_propagate(struct rb_node *node, struct rb_node *stop);
void dummy_copy(struct rb_node *old, struct rb_node *_new);
void dummy_rotate(struct rb_node *old, struct rb_node *_new);

extern const struct rb_augment_callbacks dummy_callbacks;

void
__rb_change_child(struct rb_node *old, struct rb_node *_new,
		  struct rb_node *parent, struct rb_root *root);
void
__rb_rotate_set_parents(struct rb_node *old, struct rb_node *_new,
			struct rb_root *root, int color);

void rb_link_node(struct rb_node *node, struct rb_node *parent,
				struct rb_node **rb_link);

void
__rb_insert(struct rb_node *node, struct rb_root *root,
	    bool newleft, struct rb_node **leftmost,
	    void (*augment_rotate)(struct rb_node *old, struct rb_node *_new));

void rb_insert_color_cached(struct rb_node *node,
			    struct rb_root_cached *root, bool leftmost);

struct rb_node *rb_next(const struct rb_node *node);

struct rb_node *
__rb_erase_augmented(struct rb_node *node, struct rb_root *root,
		     struct rb_node **leftmost,
		     const struct rb_augment_callbacks *augment);

void
____rb_erase_color(struct rb_node *parent, struct rb_root *root,
	void (*augment_rotate)(struct rb_node *old, struct rb_node *_new));

void rb_erase_cached(struct rb_node *node, struct rb_root_cached *root);

#define rb_first_cached(root) (root)->rb_leftmost

void debug_tasktimeline(struct rb_root_cached * tree);

#endif
