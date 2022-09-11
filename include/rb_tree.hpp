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

static inline void dummy_propagate(struct rb_node *node, struct rb_node *stop) {}
static inline void dummy_copy(struct rb_node *old, struct rb_node *_new) {}
static inline void dummy_rotate(struct rb_node *old, struct rb_node *_new) {}

static const struct rb_augment_callbacks dummy_callbacks = {
	.propagate = dummy_propagate,
	.copy = dummy_copy,
	.rotate = dummy_rotate
};

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


struct rb_node *
__rb_erase_augmented(struct rb_node *node, struct rb_root *root,
		     struct rb_node **leftmost,
		     const struct rb_augment_callbacks *augment)
{
	struct rb_node *child = node->rb_right;
	struct rb_node *tmp = node->rb_left;
	struct rb_node *parent, *rebalance;
	struct rb_node * pc_parent;
	short pc_color;

	if (leftmost && node == *leftmost)
		*leftmost = rb_next(node);

	if (!tmp) {
		/*
		 * Case 1: node to erase has no more than 1 child (easy!)
		 *
		 * Note that if there is one child it must be red due to 5)
		 * and node must be black due to 4). We adjust colors locally
		 * so as to bypass __rb_erase_color() later on.
		 */
		pc_parent = node->rb_left;
		pc_color = node->color;

		parent = pc_parent;

		__rb_change_child(node, child, parent, root);
		if (child) {
			child->rb_parent = pc_parent;
			child->color = pc_color;
			rebalance = NULL;
		} else
			rebalance = (pc_color == RB_BLACK) ? parent : NULL;
		tmp = parent;
	} else if (!child) {
		/* Still case 1, but this time the child is node->rb_left */
		tmp->rb_parent = pc_parent = node->rb_parent;
		tmp->color = pc_color = node->color;
		parent = pc_parent;
		__rb_change_child(node, tmp, parent, root);
		rebalance = NULL;
		tmp = parent;
	} else {
		struct rb_node *successor = child, *child2;

		tmp = child->rb_left;
		if (!tmp) {
			/*
			 * Case 2: node's successor is its right child
			 *
			 *    (n)          (s)
			 *    / \          / \
			 *  (x) (s)  ->  (x) (c)
			 *        \
			 *        (c)
			 */
			parent = successor;
			child2 = successor->rb_right;

			augment->copy(node, successor);
		} else {
			/*
			 * Case 3: node's successor is leftmost under
			 * node's right child subtree
			 *
			 *    (n)          (s)
			 *    / \          / \
			 *  (x) (y)  ->  (x) (y)
			 *      /            /
			 *    (p)          (p)
			 *    /            /
			 *  (s)          (c)
			 *    \
			 *    (c)
			 */
			do {
				parent = successor;
				successor = tmp;
				tmp = tmp->rb_left;
			} while (tmp);
			child2 = successor->rb_right;
			parent->rb_left = child2;
			successor->rb_right = child;
			child->rb_parent = successor;

			augment->copy(node, successor);
			augment->propagate(parent, successor);
		}

		tmp = node->rb_left;
		successor->rb_left = tmp;
		tmp->rb_parent = successor;

		pc_parent = node->rb_parent;
		pc_color = node->color;
		tmp = pc_parent;
		__rb_change_child(node, successor, tmp, root);

		if (child2) {
			successor->rb_parent = pc_parent;
			successor->color = pc_color;
			child2->rb_parent = parent;
			child2->color = RB_BLACK;
			rebalance = NULL;
		} else {
			struct rb_node * pc_parent2 = successor->rb_parent;
			short pc_color2 = successor->color;

			successor->rb_parent = pc_parent;
			successor->color = pc_color;
			rebalance = pc_color2 ? parent : NULL;
		}
		tmp = successor;
	}

	augment->propagate(tmp, NULL);
	return rebalance;
}

void
____rb_erase_color(struct rb_node *parent, struct rb_root *root,
	void (*augment_rotate)(struct rb_node *old, struct rb_node *_new))
{
	struct rb_node *node = NULL, *sibling, *tmp1, *tmp2;

	while (true) {
		/*
		 * Loop invariants:
		 * - node is black (or NULL on first iteration)
		 * - node is not the root (parent is not NULL)
		 * - All leaf paths going through parent and node have a
		 *   black node count that is 1 lower than other leaf paths.
		 */
		sibling = parent->rb_right;
		if (node != sibling) {	/* node == parent->rb_left */
			if (sibling->color == RB_RED) {
				/*
				 * Case 1 - left rotate at parent
				 *
				 *     P               S
				 *    / \             / \
				 *   N   s    -->    p   Sr
				 *      / \         / \
				 *     Sl  Sr      N   Sl
				 */
				tmp1 = sibling->rb_left;
				parent->rb_right = tmp1;
				sibling->rb_left = parent;
				tmp1->rb_parent = parent;
				tmp1->color = RB_BLACK;

				__rb_rotate_set_parents(parent, sibling, root,
							RB_RED);
				augment_rotate(parent, sibling);
				sibling = tmp1;
			}
			tmp1 = sibling->rb_right;
			if (!tmp1 || tmp1->color == RB_BLACK) {
				tmp2 = sibling->rb_left;
				if (!tmp2 || tmp2->color == RB_BLACK) {
					/*
					 * Case 2 - sibling color flip
					 * (p could be either color here)
					 *
					 *    (p)           (p)
					 *    / \           / \
					 *   N   S    -->  N   s
					 *      / \           / \
					 *     Sl  Sr        Sl  Sr
					 *
					 * This leaves us violating 5) which
					 * can be fixed by flipping p to black
					 * if it was red, or by recursing at p.
					 * p is red when coming from Case 1.
					 */
					sibling->rb_parent = parent;
					sibling->color = RB_RED;

					if (parent->color == RB_RED)
						parent->color = RB_BLACK;
					else {
						node = parent;
						parent = node->rb_parent;
						if (parent)
							continue;
					}
					break;
				}
				/*
				 * Case 3 - right rotate at sibling
				 * (p could be either color here)
				 *
				 *   (p)           (p)
				 *   / \           / \
				 *  N   S    -->  N   sl
				 *     / \             \
				 *    sl  Sr            S
				 *                       \
				 *                        Sr
				 *
				 * Note: p might be red, and then both
				 * p and sl are red after rotation(which
				 * breaks property 4). This is fixed in
				 * Case 4 (in __rb_rotate_set_parents()
				 *         which set sl the color of p
				 *         and set p RB_BLACK)
				 *
				 *   (p)            (sl)
				 *   / \            /  \
				 *  N   sl   -->   P    S
				 *       \        /      \
				 *        S      N        Sr
				 *         \
				 *          Sr
				 */
				tmp1 = tmp2->rb_right;
				sibling->rb_left = tmp1;
				tmp2->rb_right = sibling;
				parent->rb_right = tmp2;

				if (tmp1) {
					tmp1->rb_parent = sibling;
					tmp1->color = RB_BLACK;
				}
				augment_rotate(sibling, tmp2);
				tmp1 = sibling;
				sibling = tmp2;
			}
			/*
			 * Case 4 - left rotate at parent + color flips
			 * (p and sl could be either color here.
			 *  After rotation, p becomes black, s acquires
			 *  p's color, and sl keeps its color)
			 *
			 *      (p)             (s)
			 *      / \             / \
			 *     N   S     -->   P   Sr
			 *        / \         / \
			 *      (sl) sr      N  (sl)
			 */
			tmp2 = sibling->rb_left;
			parent->rb_right = tmp2;
			sibling->rb_left = parent;
			tmp1->rb_parent = sibling;
			tmp1->color = RB_BLACK;
			
			if (tmp2) {
				tmp2->rb_parent = parent;
			}
			__rb_rotate_set_parents(parent, sibling, root,
						RB_BLACK);
			augment_rotate(parent, sibling);
			break;
		} else {
			sibling = parent->rb_left;
			if (sibling->color == RB_RED) {
				/* Case 1 - right rotate at parent */
				tmp1 = sibling->rb_right;
				parent->rb_left = tmp1;
				sibling->rb_right = parent;
				tmp1->rb_parent = parent;
				tmp1->color = RB_BLACK;
				__rb_rotate_set_parents(parent, sibling, root,
							RB_RED);
				augment_rotate(parent, sibling);
				sibling = tmp1;
			}
			tmp1 = sibling->rb_left;
			if (!tmp1 || (tmp1->color == RB_BLACK)) {
				tmp2 = sibling->rb_right;
				if (!tmp2 || (tmp2->color == RB_BLACK)) {
					/* Case 2 - sibling color flip */
					sibling->rb_parent = parent;
					sibling->color = RB_RED;

					if (parent->color = RB_RED)
						parent->color = RB_BLACK;
					else {
						node = parent;
						parent = node->rb_parent;
						if (parent)
							continue;
					}
					break;
				}
				/* Case 3 - left rotate at sibling */
				tmp1 = tmp2->rb_left;
				sibling->rb_right = tmp1;
				tmp2->rb_left = sibling;
				parent->rb_left = tmp2;

				if (tmp1) {
					tmp1->rb_parent = sibling;
					tmp1->color = RB_BLACK;
				}

				augment_rotate(sibling, tmp2);
				tmp1 = sibling;
				sibling = tmp2;
			}
			/* Case 4 - right rotate at parent + color flips */
			tmp2 = sibling->rb_right;
			parent->rb_left = tmp2;
			sibling->rb_right = parent;
			tmp1->rb_parent = sibling;
			tmp1->color = RB_BLACK;
			if (tmp2) {
				tmp2->rb_parent = parent;
			}
			__rb_rotate_set_parents(parent, sibling, root,
						RB_BLACK);
			augment_rotate(parent, sibling);
			break;
		}
	}
}

void rb_erase_cached(struct rb_node *node, struct rb_root_cached *root)
{
	struct rb_node *rebalance;
	rebalance = __rb_erase_augmented(node, &root->rb_root,
					 &root->rb_leftmost, &dummy_callbacks);
	if (rebalance)
		____rb_erase_color(rebalance, &root->rb_root, dummy_rotate);
}

#define rb_first_cached(root) (root)->rb_leftmost

void debug_tasktimeline(struct rb_root_cached * tree) {
	std::cout << "    start printing the task timeline: ";
	struct rb_node * node = tree->rb_leftmost;
	for(node = tree->rb_leftmost; node != NULL; node = rb_next(node)) {
		std::cout << node->pid << ":" << node->vruntime << " ";
	}
	std::cout << std::endl;
}

#endif
