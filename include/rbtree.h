#ifndef __INCLUDED_RBTREE_H__
#define __INCLUDED_RBTREE_H__

#include "type.h"
#include "basic.h"

typedef enum{
    NODE_RED,
    NODE_BLACK,
} node_color_t;

typedef struct t_node {
    void *key;
    void *val;
    struct t_node *par;
    struct t_node *left;
    struct t_node *right;
    node_color_t color;
} node_t;

typedef struct{
    node_t *head;
    int (*comp)(void *, void *);
} tree_t;

void rbtree_init(void);
tree_t *rbtree_alloc(int (*comp)(void *,void *));
void rbtree_insert(tree_t *tree, void *key, void *val);
node_t *rbtree_search(tree_t *tree, void *key);
void rbtree_delete(tree_t *tree, void *key);

#endif
