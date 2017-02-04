/*
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#ifndef _BPLUS_TREE_H
#define _BPLUS_TREE_H

#define BPLUS_MIN_ORDER     3
#define BPLUS_MAX_ORDER     64
#define BPLUS_MAX_ENTRIES   64
#define BPLUS_MAX_LEVEL     10

struct bplus_node {
        int type;
        struct bplus_non_leaf *parent;
};

struct bplus_non_leaf {
        int type;
        struct bplus_non_leaf *parent;
        struct bplus_non_leaf *next;
        int children;
        int key[BPLUS_MAX_ORDER - 1];
        struct bplus_node *sub_ptr[BPLUS_MAX_ORDER];
};

struct bplus_leaf {
        int type;
        struct bplus_non_leaf *parent;
        struct bplus_leaf *next;
        int entries;
        int key[BPLUS_MAX_ENTRIES];
        int data[BPLUS_MAX_ENTRIES];
};

struct bplus_tree {
        int order;
        int entries;
        int level;
        struct bplus_node *root;
};

void bplus_tree_dump(struct bplus_tree *tree);
int bplus_tree_get(struct bplus_tree *tree, int key);
int bplus_tree_put(struct bplus_tree *tree, int key, int data);
int bplus_tree_get_range(struct bplus_tree *tree, int key1, int key2);
struct bplus_tree *bplus_tree_init(int level, int order, int entries);
void bplus_tree_deinit(struct bplus_tree *tree);

#endif  /* _BPLUS_TREE_H */
