//
// Created by luo-zeqi on 2023/5/26.
//

#ifndef XDOC_LLIST_H
#define XDOC_LLIST_H

typedef struct llnode {
    struct llnode *prev;
    struct llnode *next;
} llnode_t;

#define LLIST_EMPTY(root)  \
  ((root) == (root)->next)

#define LLIST_FOREACH(item, root) \
  for ((item)  = (root)->next;    \
       (item) != (root);          \
       (item)  = (item)->next)

static void llist_init(llnode_t *root) {
  root->prev = root;
  root->next = root;
}

static void llist_add(llnode_t *root, llnode_t *node) {
  root->prev->next = node;
  node->prev = root->prev;
  root->prev = node;
  node->next = root;
}

static void llist_remove(llnode_t *node) {
  node->prev->next = node->next;
  node->next->prev = node->prev;
}

static void llist_move(llnode_t *dst, llnode_t *src) {
  dst->next = src->next;
  dst->prev = src->prev;
  src->next->prev = dst;
  src->prev->next = dst;

  llist_init(src);
}

#endif //XDOC_LLIST_H
