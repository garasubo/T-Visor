#include "rbtree.h"
#include "debug.h"
#include "settings.h"

#ifndef RBTREE_NODE_POOL_NUM
#define RBTREE_NODE_POOL_NUM 1024
#endif
#ifndef RBTREE_NODE_POOL_NUM
#define RBTREE_NODE_ALLOC_NUM 32
#endif

static tree_t tree_pool[16];
static int tree_count = 0;

typedef struct {
    QUEUE que;
    node_t node;
} queue_t;

static queue_t node_pool[RBTREE_NODE_POOL_NUM];
static int pool_count = 0;

QUEUE_LIST ls;

void rbtree_init(void){
    queue_list_init(&ls);
    /* for(int i=0;i<sizeof(node_pool)/sizeof(node_pool[0]);i++){ */
    /*     queue_list_enque(&ls, (QUEUE *)&(node_pool[i])); */
    /* } */
}

tree_t *rbtree_alloc(int (*comp)(void *,void *)){
    tree_t *ret = &(tree_pool[tree_count++]);
    ret->head = NULL;
    ret->comp = comp;
    return ret;
}

static node_t *rbtree_node_alloc(void){
    if(queue_list_is_empty(&ls)){
        if(pool_count>=RBTREE_NODE_POOL_NUM){
            tv_abort("rbtree alloc failed\n");
            return NULL;
        }
        node_t *res = &(node_pool[pool_count].node);
        pool_count++;
        return res;
    }
    return &(((queue_t *)queue_list_pop(&ls))->node);
}

static void rbtree_node_free(node_t *node){
    if(node->par!=NULL){
        if(node->par->left==node) node->par->left = NULL;
        else node->par->right = NULL;
    }
    QUEUE *que = (QUEUE *)(((void *)node)-sizeof(QUEUE));
    queue_list_enque(&ls, que);
}

static node_t *rotate(node_t *tar){
    node_t *par = tar->par;
    node_t *gpar = par->par;


    if(gpar->left==par){
        if(par->left==tar){
            par->par = gpar->par;
            if(gpar->par) {
                if(gpar->par->left==gpar) gpar->par->left = par;
                else gpar->par->right = par;
            }

            gpar->par = par;
            gpar->left = par->right;
            if(par->right) par->right->par = gpar;
            par->right = gpar;
            return par;
        } else {
            tar->color = NODE_BLACK;
            void *gkey = gpar->key;
            void *gval = gpar->val;
            gpar->key = tar->key;
            gpar->val = tar->val;
            tar->key = gkey;
            tar->val = gval;
            par->right = tar->left;
            if(par->right) par->right->par = par;
            tar->left = tar->right;
            tar->right = gpar->right;
            if(tar->right) tar->right->par = tar;
            gpar->right = tar;
            gpar->right->par = gpar;
            return gpar;
        }
    } else {
        if(par->right==tar){
            void *gkey = gpar->key;
            void *gval = gpar->val;
            gpar->key = par->key;
            gpar->val = par->val;
            par->key = tar->key;
            par->val = tar->val;
            tar->key = gkey;
            tar->val = gval;
            par->right = tar->right;
            if(par->right) par->right->par = par;
            tar->right = par->left;
            if(tar->right) tar->right->par = tar;
            par->left = tar->left;
            if(par->left) par->left->par = par;
            tar->left = gpar->left;
            if(tar->left) tar->left->par = tar;
            gpar->left = tar;
            gpar->left->par = gpar;
            gpar->color = NODE_BLACK;
            par->color = NODE_RED;
            return gpar;
        } else {
            tar->color = NODE_BLACK;
            void *gkey = gpar->key;
            void *gval = gpar->val;
            gpar->key = tar->key;
            gpar->val = tar->val;
            tar->key = gkey;
            tar->val = gval;
            par->left = tar->right;
            if(par->left) par->left->par = par;
            tar->right = tar->left;
            tar->left = gpar->left;
            if(tar->left) tar->left->par = tar;
            gpar->left = tar;
            gpar->left->par = gpar;
            return gpar;
        }
    }
}



void rbtree_insert(tree_t *tree, void *key, void *val){
    node_t *tar = rbtree_node_alloc();
    tar->key = key;
    tar->val = val;
    tar->left = NULL;
    tar->right = NULL;
    tar->color = NODE_RED;

    node_t *tmp = tree->head; 
    if(tmp==NULL) {
        tar->par = NULL;
        tar->color = NODE_BLACK;
        tree->head = tar;
        return;
    }

    while(1){
        node_t *next;
        if(tree->comp(tmp->key, tar->key) > 0){
            next = tmp->right;
            if(next==NULL){
                tar->par = tmp;
                tmp->right = tar;
                break;
            }
        } else {
            next = tmp->left;
            if(next==NULL){
                tar->par = tmp;
                tmp->left = tar;
                break;
            }
        }
        tmp = next;
    }
    while(tar->color==NODE_RED&&tar->par->color==NODE_RED){
        tar->par->color = NODE_BLACK;
        tar->par->par->color = NODE_RED;
        if(tar->par->par->left==tar->par){
            if(tar->par->par->right!=NULL && tar->par->par->right->color==NODE_RED){
                tar->par->par->right->color = NODE_BLACK;
                tar = tar->par->par;
            } else {
                tar = rotate(tar);
            }
        } else {
            if(tar->par->par->left!=NULL && tar->par->par->left->color==NODE_RED){
                tar->par->par->left->color = NODE_BLACK;
                tar = tar->par->par;
            } else {
                tar = rotate(tar);
            }
        }
        if(tar->par==NULL){
            tree->head = tar;
            tar->color = NODE_BLACK;
            break;
        }
    }

}


node_t *rbtree_search(tree_t *tree, void *val){
    node_t *tar = tree->head;

    while(tar!=NULL){
        int cmp = tree->comp(tar->key, val);
        if(cmp==0) return tar;
        else if(cmp > 0){
            tar = tar->right;
        } else {
            tar = tar->left;
        }
    }
    return NULL;
}

void rbtree_delete(tree_t *tree, void *val){
    node_t *tar = rbtree_search(tree, val);
    node_t *max_node = tar->left;
    node_t *del_tar;
    if(max_node==NULL){
        if(tar->right==NULL) {
            if(tar==tree->head) tree->head = NULL;
            del_tar = tar;
        }
        else {
            tar->key = tar->right->key;
            tar->val = tar->right->val;
            del_tar = tar->right;
        }
    } else {
        while(max_node->right!=NULL){
            max_node = max_node->right;
        }
        tar->key = max_node->key;
        if(max_node->left!=NULL){
            max_node->key = max_node->left->key;
            max_node->val = max_node->left->val;
            del_tar = max_node->left;
        } else {
            del_tar = max_node;
        }
    }
    bool flag = del_tar->color==NODE_BLACK;
    tar = del_tar;
    while(flag&&tar->par!=NULL){
        node_t *tmp = tar->par;
        if(tmp->left==tar){
            node_t *v = tmp->right;
            if(v->color==NODE_BLACK){
                if(v->left!=NULL&&v->left->color==NODE_RED){
                    node_t *w = v->left;
                    flag = false;
                    void *tkey = tmp->key;
                    void *tval = tmp->val;
                    tmp->key = w->key;
                    tmp->val = w->val;
                    v->left = w->right;
                    if(w->right) w->right->par = v;
                    w->right = w->left;
                    w->left = tmp->left;
                    w->left->par = w;
                    w->par = tmp;
                    tmp->left = w;
                    w->key = tkey;
                    w->val = tval;
                    w->color = NODE_BLACK;
                }
                else if(v->right!=NULL&&v->right->color==NODE_RED){
                    node_t *w = v->right;
                    flag = false;
                    void *tkey = tmp->key;
                    void *tval = tmp->val;
                    tmp->key = v->key;
                    tmp->val = v->val;
                    w->par = tmp;
                    tmp->right = w;
                    v->right = v->left;
                    v->left = tmp->left;
                    v->left->par = v;
                    v->key = tkey;
                    v->val = tval;
                    tmp->left = v;
                    tmp->left->par = tmp;
                    w->color = NODE_BLACK;
                }
                else {
                    tmp->right->color = NODE_RED;
                    flag = tmp->color==NODE_BLACK;
                    tmp->color = NODE_BLACK;
                }
                tar = tmp;
            } else {
                void *tkey = tmp->key;
                void *tval = tmp->val;
                tmp->key = v->key;
                tmp->val = v->val;
                v->key = tkey;
                v->val = tval;
                tmp->right = v->right;
                tmp->right->par = tmp;
                v->right = v->left;
                v->left = tmp->left;
                v->left->par = v;
                tmp->left = v;
                tmp->left->par = tmp;
            }
        } else {
            node_t *v = tmp->left;
            if(v->color==NODE_BLACK){
                if(v->right!=NULL&&v->right->color==NODE_RED){
                    node_t *w = v->right;
                    flag = false;
                    void *tkey = tmp->key;
                    void *tval = tmp->val;
                    tmp->key = w->key;
                    tmp->val = w->val;
                    v->right = w->left;
                    if(v->right) v->right->par = v;
                    w->left = w->right;
                    w->right = tmp->right;
                    w->right->par = w;
                    w->par = tmp;
                    tmp->right = w;
                    w->key = tkey;
                    w->val = tval;
                    w->color = NODE_BLACK;
                }
                else if(v->left!=NULL&&v->left->color==NODE_RED){
                    node_t *w = v->left;
                    flag = false;
                    void *tkey = tmp->key;
                    void *tval = tmp->val;
                    tmp->key = v->key;
                    tmp->val = v->val;
                    w->par = tmp;
                    tmp->left = w;
                    v->left = v->right;
                    v->right = tmp->right;
                    v->right->par = v;
                    v->key = tkey;
                    v->val = tval;
                    tmp->right = v;
                    tmp->right->par = tmp;
                    w->color = NODE_BLACK;
                }
                else {
                    v->color = NODE_RED;
                    flag = tmp->color==NODE_BLACK;
                    tmp->color = NODE_BLACK;
                }
                tar = tmp;
            } else {
                void *tkey = tmp->key;
                void *tval = tmp->val;
                tmp->key = v->key;
                tmp->val = v->val;
                v->key = tkey;
                v->val = tval;
                tmp->left = v->left;
                tmp->left->par = tmp;
                v->left = v->right;
                v->right = tmp->right;
                v->right->par = v;
                tmp->right = v;
                tmp->right->par = tmp;
            }
        }
    }
    rbtree_node_free(del_tar);
}


