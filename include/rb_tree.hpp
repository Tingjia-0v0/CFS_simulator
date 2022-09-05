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
	sched_entity * se;
};

struct rb_root {
	struct rb_node *rb_node;
};

struct rb_root_cached {
	struct rb_root rb_root;
	struct rb_node *rb_leftmost;
};
static inline void dummy_rotate(struct rb_node *old, struct rb_node *_new) {}
static inline void
__rb_change_child(struct rb_node *old, struct rb_node *_new,
		  struct rb_node *parent, struct rb_root *root)
{
	if (parent) {
		if (parent->rb_left == old)
			parent->rb_left = _new;
		else
			parent->rb_right = _new;
	} else
		root->rb_node = _new;
}
static inline void
__rb_rotate_set_parents(struct rb_node *old, struct rb_node *_new,
			struct rb_root *root, int color)
{
    _new->rb_parent = old->rb_parent;
    _new->color = old->color;
    old->rb_parent = _new;
    old->color = color;

	__rb_change_child(old, _new, old->rb_parent, root);
}

void rb_link_node(struct rb_node *node, struct rb_node *parent,
				struct rb_node **rb_link)
{
	node->rb_parent = parent;
	node->rb_left = NULL;
    node->rb_right = NULL;
    node->color = RB_RED;
	*rb_link = node;
}

static void
__rb_insert(struct rb_node *node, struct rb_root *root,
	    bool newleft, struct rb_node **leftmost,
	    void (*augment_rotate)(struct rb_node *old, struct rb_node *_new))
{
    struct rb_node * parent = node->rb_parent, *gparent, *tmp;
	if (newleft)
		*leftmost = node;

	while (true) {
		/*
		 * Loop invariant: node is red.
		 */
		if (!parent) {
			/*
			 * The inserted node is root. Either this is the
			 * first node, or we recursed at Case 1 below and
			 * are no longer violating 4).
			 */
            node->rb_parent = NULL;
            node->color = RB_BLACK;
			break;
		}

		/*
		 * If there is a black parent, we are done.
		 * Otherwise, take some corrective action as,
		 * per 4), we don't want a red root or two
		 * consecutive red nodes.
		 */
		if(parent->color)
			break;

		gparent = parent->rb_parent;

		tmp = gparent->rb_right;
		if (parent != tmp) {	/* parent == gparent->rb_left */
			if (tmp && (!tmp->color)) {
				/*
				 * Case 1 - node's uncle is red (color flips).
				 *
				 *       G            g
				 *      / \          / \
				 *     p   u  -->   P   U
				 *    /            /
				 *   n            n
				 *
				 * However, since g's parent might be red, and
				 * 4) does not allow this, we need to recurse
				 * at g.
				 */
                tmp->rb_parent = gparent;
                tmp->color = RB_BLACK;
                parent->rb_parent = gparent;
                parent->color = RB_BLACK;

				node = gparent;
				parent = node->rb_parent;
                node->rb_parent = parent;
                node->color = RB_RED;
				continue;
			}

			tmp = parent->rb_right;
			if (node == tmp) {
				/*
				 * Case 2 - node's uncle is black and node is
				 * the parent's right child (left rotate at parent).
				 *
				 *      G             G
				 *     / \           / \
				 *    p   U  -->    n   U
				 *     \           /
				 *      n         p
				 *
				 * This still leaves us in violation of 4), the
				 * continuation into Case 3 will fix that.
				 */
				tmp = node->rb_left;
                parent->rb_right = tmp;
                node->rb_left = parent;
				if (tmp) {
                    tmp->rb_parent = parent;
                    tmp->color = RB_BLACK;
                }
                parent->rb_parent = node;
                parent->color = RB_RED;

				augment_rotate(parent, node);
				parent = node;
				tmp = node->rb_right;
			}

			/*
			 * Case 3 - node's uncle is black and node is
			 * the parent's left child (right rotate at gparent).
			 *
			 *        G           P
			 *       / \         / \
			 *      p   U  -->  n   g
			 *     /                 \
			 *    n                   U
			 */
            gparent->rb_left = tmp;
            parent->rb_right = gparent;

			if (tmp) {
                tmp->rb_parent = gparent;
                tmp->color = RB_BLACK;
            }
			__rb_rotate_set_parents(gparent, parent, root, RB_RED);
			augment_rotate(gparent, parent);
			break;
		} else {
			tmp = gparent->rb_left;
			if (tmp && (!tmp->color)) {
				/* Case 1 - color flips */
                tmp->rb_parent = gparent;
                tmp->color = RB_BLACK;
                parent->rb_parent = gparent;
                parent->color = RB_BLACK;
                
				node = gparent;
				parent = node->rb_parent;
                node->rb_parent = parent;
                node->color = RB_RED;
				continue;
			}

			tmp = parent->rb_left;
			if (node == tmp) {
				/* Case 2 - right rotate at parent */
				tmp = node->rb_right;
                parent->rb_left = tmp;
                node->rb_right = parent;
				if (tmp) {
                    tmp->rb_parent = parent;
                    tmp->color = RB_BLACK;
                }

                parent->rb_parent = node;
                parent->color = RB_RED;

				augment_rotate(parent, node);
				parent = node;
				tmp = node->rb_left;
			}

			/* Case 3 - left rotate at gparent */
            gparent->rb_right = tmp;
            parent->rb_left = gparent;
			if (tmp) {
                tmp->rb_parent = gparent;
                tmp->color = RB_BLACK;
            }
			__rb_rotate_set_parents(gparent, parent, root, RB_RED);
			augment_rotate(gparent, parent);
			break;
		}
	}
}


void rb_insert_color_cached(struct rb_node *node,
			    struct rb_root_cached *root, bool leftmost)
{
	__rb_insert(node, &root->rb_root, leftmost,
		    &root->rb_leftmost, dummy_rotate);
}

struct rb_node *rb_next(const struct rb_node *node)
{
	struct rb_node *parent;

	if (node == NULL)
		return NULL;

	/*
	 * If we have a right-hand child, go down and then left as far
	 * as we can.
	 */
	if (node->rb_right) {
		node = node->rb_right;
		while (node->rb_left)
			node=node->rb_left;
		return (struct rb_node *)node;
	}

	/*
	 * No right-hand children. Everything down and left is smaller than us,
	 * so any 'next' node must be in the general direction of our parent.
	 * Go up the tree; any time the ancestor is a right-hand child of its
	 * parent, keep going up. First time it's a left-hand child of its
	 * parent, said parent is our 'next' node.
	 */
	while ((parent = node->rb_parent) && node == parent->rb_right)
		node = parent;

	return parent;
}

void debug_tasktimeline(struct rb_root_cached * tree) {
	std::cout << "start printing the task timeline: " << std::endl;
	struct rb_node * node = tree->rb_leftmost;
	for(node = tree->rb_leftmost; node != NULL; node = rb_next(node)) {
		std::cout << node->vruntime << " ";
	}
	std::cout << std::endl;
}
#endif
