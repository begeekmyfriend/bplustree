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

struct bplus_node {
        int type;
        int parent_key_idx;
        struct bplus_non_leaf *parent;
        struct list_head link;
        int count;
};

struct bplus_non_leaf {
        int type;
        int parent_key_idx;
        struct bplus_non_leaf *parent;
        struct list_head link;
        int children;
        int key[BPLUS_MAX_ORDER - 1];
        struct bplus_node *sub_ptr[BPLUS_MAX_ORDER];
};

struct bplus_leaf {
        int type;
        int parent_key_idx;
        struct bplus_non_leaf *parent;
        struct list_head link;
        int entries;
        int key[BPLUS_MAX_ENTRIES];
        int data[BPLUS_MAX_ENTRIES];
};

struct bplus_tree {
        int order;
        int entries;
        int level;
        struct bplus_node *root;
        struct list_head list[BPLUS_MAX_LEVEL];
};

void bplus_tree_dump(struct bplus_tree *tree);
int bplus_tree_get(struct bplus_tree *tree, key_t key);
int bplus_tree_put(struct bplus_tree *tree, key_t key, int data);
int bplus_tree_get_range(struct bplus_tree *tree, key_t key1, key_t key2);
struct bplus_tree *bplus_tree_init(int order, int entries);
void bplus_tree_deinit(struct bplus_tree *tree);

#endif  /* _BPLUS_TREE_H */
