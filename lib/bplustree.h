/*
 * Copyright (C) 2017, Leo Ma <begeekmyfriend@gmail.com>
 */

#ifndef _BPLUS_TREE_H
#define _BPLUS_TREE_H

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

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

#define list_entry(ptr, type, member) \
        ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)

typedef struct free_block {
        struct list_head link;
        off_t offset;
} free_block;

typedef struct node_cache {
        struct list_head link;
        char *buf;
} free_cache;

typedef struct bplus_node {
        struct node_cache *cache;
        off_t self;
        off_t parent;
        off_t prev;
        off_t next;
        int type;
        int parent_key_idx;
        /* If leaf node, it specifies  count of entries,
         * if non-leaf node, it specifies count of children(branches) */
        int children;
        int reserve;
} bplus_node;
/*
struct bplus_non_leaf {
        struct node_cache *cache;
        off_t self;
        off_t parent;
        off_t prev;
        off_t next;
        int type;
        int parent_key_idx;
        int children;
        int reserve;
        int key[BPLUS_MAX_ORDER - 1];
        off_t sub_ptr[BPLUS_MAX_ORDER];
};

struct bplus_leaf {
        struct node_cache *cache;
        off_t self;
        off_t parent;
        off_t prev;
        off_t next;
        int type;
        int parent_key_idx;
        int entries;
        int reserve;
        int key[BPLUS_MAX_ENTRIES];
        long data[BPLUS_MAX_ENTRIES];
};
*/
struct bplus_tree {
        char filename[1024];
        int fd;
        int order;
        int entries;
        int level;
        int block_size;
        off_t root;
        off_t file_size;
        struct list_head free_blocks;
        struct list_head free_caches;
};

void bplus_tree_dump(struct bplus_tree *tree);
long bplus_tree_get(struct bplus_tree *tree, int key);
int bplus_tree_put(struct bplus_tree *tree, int key, long data);
long bplus_tree_get_range(struct bplus_tree *tree, int key1, int key2);
struct bplus_tree *bplus_tree_init(char *filename, int block_size);
void bplus_tree_deinit(struct bplus_tree *tree);
int bplus_open(char *filename);
void bplus_close(int fd);

#endif  /* _BPLUS_TREE_H */
