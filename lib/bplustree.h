/*
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#ifndef _BPLUS_TREE_H
#define _BPLUS_TREE_H

#define BPLUS_MIN_ORDER     3
#define BPLUS_MAX_ORDER     64
#define BPLUS_MAX_ENTRIES   64
#define BPLUS_MAX_LEVEL     10

struct bplus_link {
        struct bplus_link *prev, *next;
};

static inline void list_init(struct bplus_link *link)
{
        link->prev = link;
        link->next = link;
}

static inline void
__list_add(struct bplus_link *link, struct bplus_link *prev, struct bplus_link *next)
{
        link->next = next;
        link->prev = prev;
        next->prev = link;
        prev->next = link;
}

static inline void __list_del(struct bplus_link *prev, struct bplus_link *next)
{
        prev->next = next;
        next->prev = prev;
}

static inline void list_add(struct bplus_link *link, struct bplus_link *prev)
{
        __list_add(link, prev, prev->next);
}

static inline void list_del(struct bplus_link *link)
{
        __list_del(link->prev, link->next);
        list_init(link);
}

#define list_entry(ptr, type, member) \
        ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

#define bplus_foreach(pos, end) \
        for (; pos != end; pos = pos->next)

#define bplus_foreach_safe(pos, n, end) \
        for (n = pos->next; pos != end; pos = n, n = pos->next)

struct bplus_node {
        int type;
        struct bplus_non_leaf *parent;
        struct bplus_link link;
};

struct bplus_non_leaf {
        int type;
        struct bplus_non_leaf *parent;
        struct bplus_link link;
        int children;
        int key[BPLUS_MAX_ORDER - 1];
        struct bplus_node *sub_ptr[BPLUS_MAX_ORDER];
};

struct bplus_leaf {
        int type;
        struct bplus_non_leaf *parent;
        struct bplus_link link;
        int entries;
        int key[BPLUS_MAX_ENTRIES];
        int data[BPLUS_MAX_ENTRIES];
};

struct bplus_tree {
        int order;
        int entries;
        int level;
        struct bplus_node *root;
        struct bplus_link list[BPLUS_MAX_LEVEL];
};

void bplus_tree_dump(struct bplus_tree *tree);
int bplus_tree_get(struct bplus_tree *tree, int key);
int bplus_tree_put(struct bplus_tree *tree, int key, int data);
int bplus_tree_get_range(struct bplus_tree *tree, int key1, int key2);
struct bplus_tree *bplus_tree_init(int order, int entries);
void bplus_tree_deinit(struct bplus_tree *tree);

#endif  /* _BPLUS_TREE_H */
