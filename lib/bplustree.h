/*
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#ifndef _BPLUS_TREE_H
#define _BPLUS_TREE_H

#define BPLUS_MIN_ORDER     3
#define BPLUS_MAX_ORDER     64
#define BPLUS_MAX_ENTRIES   64
#define BPLUS_MAX_LEVEL     10

typedef int key_t;

struct list_head {
        struct list_head *prev, *next;
};

static inline void list_init(struct list_head *link)
{
        link->prev = link;
        link->next = link;
}

static inline void
__list_add(struct list_head *link, struct list_head *prev, struct list_head *next)
{
        link->next = next;
        link->prev = prev;
        next->prev = link;
        prev->next = link;
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
        prev->next = next;
        next->prev = prev;
}

static inline void list_add(struct list_head *link, struct list_head *prev)
{
        __list_add(link, prev, prev->next);
}

static inline void list_add_tail(struct list_head *link, struct list_head *head)
{
	__list_add(link, head->prev, head);
}

static inline void list_del(struct list_head *link)
{
        __list_del(link->prev, link->next);
        list_init(link);
}

static inline int list_is_first(struct list_head *link, struct list_head *head)
{
	return link->prev == head;
}

static inline int list_is_last(struct list_head *link, struct list_head *head)
{
	return link->next == head;
}

#define list_entry(ptr, type, member) \
        ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)
/**
 * b plus tree basic node
 * 
 * 
 */ 
struct bplus_node {
/**  type indicates leaf node or not
 *     BPLUS_TREE_LEAF is 0 and  BPLUS_TREE_NON_LEAF is 1
 */
        int type;
/**     parent_key_idx: index of parent node */
        int parent_key_idx;
/** piointer to parent node */
        struct bplus_non_leaf *parent;
/** pointer to first node(head) in leaf linked list */
        struct list_head link;
/** */
        int count;
};
/**  b plus tree non-leaf(internal)  node 
 * non-leaf node need to carry children node information
 * node contains only keys not key-value-pairs
 
*/
struct bplus_non_leaf {
/**  type indicates leaf node or not
 *     BPLUS_TREE_LEAF is 0 and  BPLUS_TREE_NON_LEAF is 1
 */
        int type;
/**     parent_key_idx: index of parent node */
        int parent_key_idx;
/** piointer to parent node */
        struct bplus_non_leaf *parent;
/** pointer to first node(head) in leaf linked list 
*/
        struct list_head link;
/**  number of child node */
        int children;
/**  key array */
        int key[BPLUS_MAX_ORDER - 1];
/** pointers to child node */
        struct bplus_node *sub_ptr[BPLUS_MAX_ORDER];
};
/**  b plus tree leaf node 
 * leaf node need to carry key-value-pairs
 
*/
struct bplus_leaf {
/**  type indicates leaf node or not
 *     BPLUS_TREE_LEAF is 0 and  BPLUS_TREE_NON_LEAF is 1
 */
        int type;
/**     parent_key_idx: index of parent node */
        int parent_key_idx;
/** piointer to parent node */
        struct bplus_non_leaf *parent;
/** pointer to first node(head) in leaf linked list 
*/
        struct list_head link;
/** number of actual key-value pairs in leaf node */
        int entries;
/**  key array */
        int key[BPLUS_MAX_ENTRIES];
/**  val array */
        int data[BPLUS_MAX_ENTRIES];

};
/** b plus tree structure */
struct bplus_tree {
/**  The actual number of children for a node, referred to here as order */
        int order;
/** number of actual key-value pairs in tree */
        int entries;
/** height of the tree */
        int level;
        struct bplus_node *root;
/** double linked list to search leaf node */
        struct list_head list[BPLUS_MAX_LEVEL];
};
 /** print the whole tree for debugging
   *  
   * @param tree pointer to bplus tree
   */
void bplus_tree_dump(struct bplus_tree *tree);
 /** return value acordding to key
   *  
   * @param tree pointer to bplus tree
   * @param key  key in key-value pair
   */
int bplus_tree_get(struct bplus_tree *tree, key_t key);
 /** insert key-value pair to tree
   *  
   * @param tree pointer to bplus tree
   * @param key  key in key-value pair
   * @param data value in key-value pair
   
   */
int bplus_tree_put(struct bplus_tree *tree, key_t key, int data);

 /** return data between [key1,key2]
   *  
   * @param tree pointer to bplus tree
   * @param key1  key in key-value pair
   * @param key2 value in key-value pair
   
*/
int bplus_tree_get_range(struct bplus_tree *tree, key_t key1, key_t key2);

 /** init b plus tree 
  * @return a pointer to tree
   *  
   * @param order  The actual number of children for a node, referred to here as order 
   * @param key1  key in key-value pair
   * @param key2  key in key-value pair
   
*/
struct bplus_tree *bplus_tree_init(int order, int entries);
 /** destory the tree safely
  * @return a pointer to tree
   *  
   * @param order  The actual number of children for a node, referred to here as order 
   * @param entries   number of actual key-value pairs in tree 
   
   
*/
void bplus_tree_deinit(struct bplus_tree *tree);

#endif  /* _BPLUS_TREE_H */



