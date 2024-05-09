//
// Created by luo-zeqi on 2023/5/26.
//

#ifndef XDOC_LLIST_H
#define XDOC_LLIST_H

//typedef struct llnode {
//    struct llnode *prev;
//    struct llnode *next;
//} llnode_t;

#define DECLARE_LLIST_NODE(type)      \
  type *prev;                         \
  type *next;

#define LLIST_EMPTY(root)  \
  ((root) == (root)->next)

#define LLIST_FOREACH(item, root) \
  for ((item)  = ((root)->next);  \
       (item) != (root);        \
       (item)  = (item)->next)

#define LLIST_INIT(root) do { \
  (root)->prev = (root);          \
  (root)->next = (root);          \
} while (0)

#define LLIST_ADD(root, node) do { \
  (root)->prev->next = (node);         \
  (node)->prev = (root)->prev;         \
  (root)->prev = (node);               \
  (node)->next = (root);               \
} while (0)

#define LLIST_INSERT(target, node) do { \
  (node)->next = (target)->next;        \
  (node)->prev = (target);              \
  (target)->next->prev = (node);        \
  (target)->next = (node);              \
} while (0)

#define LLIST_REMOVE(node) do {      \
  (node)->prev->next = (node)->next; \
  (node)->next->prev = (node)->prev; \
} while (0)


//static \
//void prefix##llist_move(type *dst,      \
//                     type *src) {       \
//  dst->next = src->next;                \
//  dst->prev = src->prev;                \
//  src->next->prev = dst;                \
//  src->prev->next = dst;                \
//  prefix##llist_init(src);              \
//}


#endif //XDOC_LLIST_H
