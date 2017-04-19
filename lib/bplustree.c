/*
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bplustree.h"

enum {
        INVALID_OFFSET = -1,
};

enum {
        BPLUS_TREE_LEAF,
        BPLUS_TREE_NON_LEAF = 1,
};

enum {
        LEFT_SIBLING,
        RIGHT_SIBLING = 1,
};

#define offset_ptr(node) ((char *)node + sizeof(*node))
#define key(node) ((int *)offset_ptr(node))
#define data(node) ((long *)(offset_ptr(node) + max_entries * sizeof(int)))
#define sub(node) ((off_t *)(offset_ptr(node) + (max_order - 1) * sizeof(int)))

static int max_order;
static int max_entries;

static inline int
is_leaf(struct bplus_node *node)
{
        return node->type == BPLUS_TREE_LEAF;
}

static int
key_binary_search(struct bplus_node *node, int target)
{
        int *arr = key(node);
        int len = is_leaf(node) ? node->count : node->count - 1;
        int low = -1;
        int high = len;

        while (low + 1 < high) {
                int mid = low + (high - low) / 2;
                if (target > arr[mid]) {
                        low = mid;
                } else {
                        high = mid;
                }
        }

        if (high >= len || arr[high] != target) {
                return -high - 1;
        } else {
                return high;
        }
}

static struct bplus_node *
node_new(struct bplus_tree *tree)
{
        struct free_cache *cache;
        assert(!list_empty(&tree->free_caches));
        cache = list_first_entry(&tree->free_caches, struct free_cache, link);
        list_del(&cache->link);
        struct bplus_node *node = (struct bplus_node *)cache->buf;
        node->cache = cache;
        node->prev = INVALID_OFFSET;
        node->next = INVALID_OFFSET;
        node->self = INVALID_OFFSET;
        node->parent = INVALID_OFFSET;
        node->parent_key_idx = -1;
        node->count = 0;
        return node;
}

static struct bplus_node *
non_leaf_new(struct bplus_tree *tree)
{
        struct bplus_node *node = node_new(tree);
        node->type = BPLUS_TREE_NON_LEAF;
        return node;
}

static struct bplus_node *
leaf_new(struct bplus_tree *tree)
{
        struct bplus_node *node = node_new(tree);
        node->type = BPLUS_TREE_LEAF;
        return node;
}

static struct bplus_node *
node_fetch(struct bplus_tree *tree, off_t offset)
{
        if (offset == INVALID_OFFSET) {
                return NULL;
        }

        struct free_cache *cache;
        assert(!list_empty(&tree->free_caches));
        cache = list_first_entry(&tree->free_caches, struct free_cache, link);
        list_del(&cache->link);
        int len = pread(tree->fd, cache->buf, tree->block_size, offset);
        assert(len == tree->block_size);
        struct bplus_node *node = (struct bplus_node *)cache->buf;
        node->cache = cache;
        return node;
}

static struct bplus_node *
node_seek(struct bplus_tree *tree, off_t offset)
{
        if (offset == INVALID_OFFSET) {
                return NULL;
        }

        struct free_cache *cache;
        assert(!list_empty(&tree->free_caches));
        cache = list_first_entry(&tree->free_caches, struct free_cache, link);
        int len = pread(tree->fd, cache->buf, tree->block_size, offset);
        assert(len == tree->block_size);
        struct bplus_node *node = (struct bplus_node *)cache->buf;
        node->cache = cache;
        return node;
}

static void
node_write_back(struct bplus_tree *tree, struct bplus_node *node)
{
        struct free_cache *cache = node->cache;
        /* node cache flush with its offset remains the same */
        int len = pwrite(tree->fd, cache->buf, tree->block_size, node->self);
        assert(len == tree->block_size);
}

static void
node_flush(struct bplus_tree *tree, struct bplus_node *node)
{
        struct free_cache *cache = node->cache;
        /* node cache flush with its offset remains the same */
        int len = pwrite(tree->fd, cache->buf, tree->block_size, node->self);
        assert(len == tree->block_size);
        /* return the cache borrowed from */
        node->cache = NULL;
        list_add_tail(&cache->link, &tree->free_caches);
}

static off_t
new_node_write(struct bplus_tree *tree, struct bplus_node *node)
{
        /* assign new offset to the new node */
        if (list_empty(&tree->free_blocks)) {
                node->self = tree->file_offset;
                tree->file_offset += tree->block_size;
        } else {
                struct free_block *block;
                block = list_first_entry(&tree->free_blocks, struct free_block, link);
                list_del(&block->link);
                node->self = block->offset;
                free(block);
        }
        node_write_back(tree, node);
        return node->self;
}

static void
node_delete(struct bplus_tree *tree, struct bplus_node *node,
                struct bplus_node *left, struct bplus_node *right)
{
        if (left != NULL) {
                if (right != NULL) {
                        left->next = right->self;
                        right->prev = left->self;
                        node_flush(tree, right);
                } else {
                        left->next = INVALID_OFFSET;
                }
                node_flush(tree, left);
        } else {
                if (right != NULL) {
                        right->prev = INVALID_OFFSET;
                        node_flush(tree, right);
                }
        }

        assert(node->self != INVALID_OFFSET);
        struct free_block *block = malloc(sizeof(*block));
        assert(block != NULL);
        /* deleted block can be reused for other nodes */
        block->offset = node->self;
        list_add_tail(&block->link, &tree->free_blocks);

        /* return the cache borrowed from */
        struct free_cache *cache = node->cache;
        node->self = INVALID_OFFSET;
        node->parent = INVALID_OFFSET;
        node->prev = INVALID_OFFSET;
        node->next = INVALID_OFFSET;
        node->cache = NULL;
        list_add_tail(&cache->link, &tree->free_caches);
}

static void
sub_node_flush(struct bplus_tree *tree, struct bplus_node *parent,
                off_t sub_offset, int key_idx)
{
        struct bplus_node *sub_node = node_fetch(tree, sub_offset);
        assert(sub_node != NULL);
        sub_node->parent = parent->self;
        sub_node->parent_key_idx = key_idx;
        node_flush(tree, sub_node);
}

static long
bplus_tree_search(struct bplus_tree *tree, int key)
{
        int ret = -1;
        struct bplus_node *node = node_seek(tree, tree->root);
        while (node != NULL) {
                int i = key_binary_search(node, key);
                if (is_leaf(node)) {
                        ret = i >= 0 ? data(node)[i] : -1;
                        break;
                } else {
                        if (i >= 0) {
                                node = node_seek(tree, sub(node)[i + 1]);
                        } else {
                                i = -i - 1;
                                node = node_seek(tree, sub(node)[i]);
                        }
                }
        }
        return ret;
}

static void
left_node_add(struct bplus_tree *tree, struct bplus_node *node, struct bplus_node *left)
{
        new_node_write(tree, left);
        struct bplus_node *prev = node_fetch(tree, node->prev);
        if (prev != NULL) {
                prev->next = left->self;
                left->prev = prev->self;
                node_flush(tree, prev);
        } else {
                left->prev = INVALID_OFFSET;
        }
        left->next = node->self;
        node->prev = left->self;
}

static void
right_node_add(struct bplus_tree *tree, struct bplus_node *node, struct bplus_node *right)
{
        new_node_write(tree, right);
        struct bplus_node *next = node_fetch(tree, node->next);
        if (next != NULL) {
                next->prev = right->self;
                right->next = next->self;
                node_flush(tree, next);
        } else {
                right->next = INVALID_OFFSET;
        }
        right->prev = node->self;
        node->next = right->self;
}

static int non_leaf_insert(struct bplus_tree *tree, struct bplus_node *node,
                struct bplus_node *l_ch, struct bplus_node *r_ch, int key, int level);

static int
parent_node_build(struct bplus_tree *tree, struct bplus_node *l_ch,
                struct bplus_node *r_ch, int key, int level)
{
        if (l_ch->parent == INVALID_OFFSET && r_ch->parent == INVALID_OFFSET) {
                /* new parent */
                struct bplus_node *parent = non_leaf_new(tree);
                key(parent)[0] = key;
                sub(parent)[0] = l_ch->self;
                sub(parent)[1] = r_ch->self;
                parent->count = 2;
                /* write new parent and update root */
                tree->root = new_node_write(tree, parent);
                l_ch->parent = tree->root;
                l_ch->parent_key_idx = -1;
                r_ch->parent = tree->root;
                r_ch->parent_key_idx = 0;
                /* parent, left and right child cache flush */
                node_flush(tree, l_ch);
                node_flush(tree, r_ch);
                node_flush(tree, parent);
                return 0;
        } else if (r_ch->parent == INVALID_OFFSET) {
                r_ch->parent = l_ch->parent;
                /* left child cache flush */
                node_flush(tree, l_ch);
                node_flush(tree, r_ch);
                /* trace upwards */
                return non_leaf_insert(tree, node_fetch(tree, l_ch->parent),
                                        l_ch, r_ch, key, level + 1);
        } else {
                l_ch->parent = r_ch->parent;
                /* right child cache flush */
                node_flush(tree, l_ch);
                node_flush(tree, r_ch);
                /* trace upwards */
                return non_leaf_insert(tree, node_fetch(tree, r_ch->parent),
                                        l_ch, r_ch, key, level + 1);
        }
}

static void
non_leaf_simple_insert(struct bplus_tree *tree, struct bplus_node *node,
                        struct bplus_node *l_ch, struct bplus_node *r_ch,
                        int key, int insert)
{
        int i;
        for (i = node->count - 1; i > insert; i--) {
                key(node)[i] = key(node)[i - 1];
                sub(node)[i + 1] = sub(node)[i];
                sub_node_flush(tree, node, sub(node)[i + 1], i);
        }
        key(node)[i] = key;
        sub(node)[i] = l_ch->self;
        sub_node_flush(tree, node, sub(node)[i], i - 1);
        sub(node)[i + 1] = r_ch->self;
        sub_node_flush(tree, node, sub(node)[i + 1], i);
        node->count++;
        /* node cache flush */
        node_flush(tree, node);
}

static int
non_leaf_split_left(struct bplus_tree *tree, struct bplus_node *node,
                struct bplus_node *left, struct bplus_node *l_ch,
                struct bplus_node *r_ch, int key, int insert)
{
        int i, j, split_key;
        /* split = [m/2] */
        int split = (tree->order + 1) / 2;
        /* split as left sibling */
        left_node_add(tree, node, left);
        /* replicate from sub[0] to sub[split - 1] */
        for (i = 0, j = 0; i < split; i++, j++) {
                if (j == insert) {
                        sub(left)[j] = l_ch->self;
                        sub_node_flush(tree, left, sub(left)[j], j - 1);
                        sub(left)[j + 1] = r_ch->self;
                        sub_node_flush(tree, left, sub(left)[j + 1], j);
                        j++;
                } else {
                        sub(left)[j] = sub(node)[i];
                        sub_node_flush(tree, left, sub(left)[j], j - 1);
                }
        }
        left->count = split;
        /* replicate from key[0] to key[split - 2] */
        for (i = 0, j = 0; i < split - 1; j++) {
                if (j == insert) {
                        key(left)[j] = key;
                } else {
                        key(left)[j] = key(node)[i];
                        i++;
                }
        }
        if (insert == split - 1) {
                key(left)[insert] = key;
                sub(left)[insert] = l_ch->self;
                sub_node_flush(tree, left, sub(left)[insert], insert - 1);
                sub(node)[0] = r_ch->self;
                split_key = key;
        } else {
                sub(node)[0] = sub(node)[split - 1];
                split_key = key(node)[split - 2];
        }
        sub_node_flush(tree, node, sub(node)[0], -1);
        /* left shift for right node from split - 1 to children - 1 */
        for (i = split - 1, j = 0; i < tree->order - 1; i++, j++) {
                key(node)[j] = key(node)[i];
                sub(node)[j + 1] = sub(node)[i + 1];
                sub_node_flush(tree, node, sub(node)[j + 1], j);
        }
        sub(node)[j] = sub(node)[i];
        sub_node_flush(tree, node, sub(node)[j], j - 1);
        node->count = j + 1;
        return split_key;
}

static int
non_leaf_split_right1(struct bplus_tree *tree, struct bplus_node *node,
                        struct bplus_node *right, struct bplus_node *l_ch,
                        struct bplus_node *r_ch, int key, int insert)
{
        int i, j, split_key = key;
        /* split = [m/2] */
        int split = (tree->order + 1) / 2;
        /* split as right sibling */
        right_node_add(tree, node, right);
        /* left node's children always split + 1 */
        node->count = split + 1;
        sub(node)[split] = l_ch->self;
        sub_node_flush(tree, node, sub(node)[split], split - 1);
        /* right node's first sub-node */
        sub(right)[0] = r_ch->self;
        sub_node_flush(tree, right, sub(right)[0], -1);
        /* insertion point is split point, replicate from key[split] */
        for (i = split, j = 0; i < tree->order - 1; i++, j++) {
                key(right)[j] = key(node)[i];
                sub(right)[j + 1] = sub(node)[i + 1];
                sub_node_flush(tree, right, sub(right)[j + 1], j);
        }
        right->count = j + 1;
        return split_key;
}

static int
non_leaf_split_right2(struct bplus_tree *tree, struct bplus_node *node,
                        struct bplus_node *right, struct bplus_node *l_ch,
                        struct bplus_node *r_ch, int key, int insert)
{
        int i, j, split_key;
        /* split = [m/2] */
        int split = (tree->order + 1) / 2;
        /* left node's children always split + 1 */
        node->count = split + 1;
        /* split as right sibling */
        right_node_add(tree, node, right);
        split_key = key(node)[split];
        /* right node's first sub-node */
        sub(right)[0] = sub(node)[split + 1];
        sub_node_flush(tree, right, sub(right)[0], -1);
        /* replicate from key[split + 1] to key[order - 1] */
        for (i = split + 1, j = 0; i < tree->order - 1; j++) {
                if (j != insert - split - 1) {
                        key(right)[j] = key(node)[i];
                        sub(right)[j + 1] = sub(node)[i + 1];
                        sub_node_flush(tree, right, sub(right)[j + 1], j);
                        i++;
                }
        }
        /* reserve a hole for insertion */
        if (j > insert - split - 1) {
                right->count = j + 1;
        } else {
                assert(j == insert - split - 1);
                right->count = j + 2;
        }
        /* insert new key and sub-node */
        j = insert - split - 1;
        key(right)[j] = key;
        sub(right)[j] = l_ch->self;
        sub_node_flush(tree, right, sub(right)[j], j - 1);
        sub(right)[j + 1] = r_ch->self;
        sub_node_flush(tree, right, sub(right)[j + 1], j);
        return split_key;
}

static int
non_leaf_insert(struct bplus_tree *tree, struct bplus_node *node,
                struct bplus_node *l_ch, struct bplus_node *r_ch, int key, int level)
{
        /* Search key location */
        int insert = key_binary_search(node, key);
        assert(insert < 0);
        insert = -insert - 1;

        /* node is full */
        if (node->count == tree->order) {
                int split_key;
                /* split = [m/2] */
                int split = (node->count + 1) / 2;
                struct bplus_node *sibling = non_leaf_new(tree);
                if (insert < split) {
                        split_key = non_leaf_split_left(tree, node, sibling, l_ch, r_ch, key, insert);
                } else if (insert == split) {
                        split_key = non_leaf_split_right1(tree, node, sibling, l_ch, r_ch, key, insert);
                } else {
                        split_key = non_leaf_split_right2(tree, node, sibling, l_ch, r_ch, key, insert);
                }
                /* build new parent */
                if (insert < split) {
                        return parent_node_build(tree, sibling, node, split_key, level);
                } else {
                        return parent_node_build(tree, node, sibling, split_key, level);
                }
        } else {
                non_leaf_simple_insert(tree, node, l_ch, r_ch, key, insert);
        }
        return 0;
}

static void
leaf_simple_insert(struct bplus_tree *tree, struct bplus_node *leaf,
                int key, long data, int insert)
{
        int i;
        for (i = leaf->count; i > insert; i--) {
                key(leaf)[i] = key(leaf)[i - 1];
                data(leaf)[i] = data(leaf)[i - 1];
        }
        key(leaf)[i] = key;
        data(leaf)[i] = data;
        leaf->count++;
        /* leaf cache flush */
        node_flush(tree, leaf);
}

static void
leaf_split_left(struct bplus_tree *tree, struct bplus_node *leaf,
                struct bplus_node *left, int key, long data, int insert)
{
        int i, j;
        /* split = [m/2] */
        int split = (leaf->count + 1) / 2;
        /* split as left sibling */
        left_node_add(tree, leaf, left);
        /* replicate from 0 to key[split - 2] */
        for (i = 0, j = 0; i < split - 1; j++) {
                if (j == insert) {
                        key(left)[j] = key;
                        data(left)[j] = data;
                } else {
                        key(left)[j] = key(leaf)[i];
                        data(left)[j] = data(leaf)[i];
                        i++;
                }
        }
        if (j == insert) {
                key(left)[j] = key;
                data(left)[j] = data;
                j++;
        }
        left->count = j;
        /* left shift for right node */
        for (j = 0; i < leaf->count; i++, j++) {
                key(leaf)[j] = key(leaf)[i];
                data(leaf)[j] = data(leaf)[i];
        }
        leaf->count = j;
}

static void
leaf_split_right(struct bplus_tree *tree, struct bplus_node *leaf,
                struct bplus_node *right, int key, long data, int insert)
{
        int i, j;
        /* split = [m/2] */
        int split = (leaf->count + 1) / 2;
        /* split as right sibling */
        right_node_add(tree, leaf, right);
        /* replicate from key[split] */
        for (i = split, j = 0; i < leaf->count; j++) {
                if (j != insert - split) {
                        key(right)[j] = key(leaf)[i];
                        data(right)[j] = data(leaf)[i];
                        i++;
                }
        }
        /* reserve a hole for insertion */
        if (j > insert - split) {
                right->count = j;
        } else {
                assert(j == insert - split);
                right->count = j + 1;
        }
        /* insert new key */
        j = insert - split;
        key(right)[j] = key;
        data(right)[j] = data;
        /* left leaf number */
        leaf->count = split;
}

static int
leaf_insert(struct bplus_tree *tree, struct bplus_node *leaf, int key, long data)
{
        /* Search key location */
        int insert = key_binary_search(leaf, key);
        if (insert >= 0) {
                /* Already exists */
                return -1;
        }
        insert = -insert - 1;

        /* fetch from free cache list */
        list_del(&leaf->cache->link);

        /* leaf is full */
        if (leaf->count == tree->entries) {
                /* split = [m/2] */
                int split = (tree->entries + 1) / 2;
                struct bplus_node *sibling = leaf_new(tree);
                /* sibling leaf replication due to location of insertion */
                if (insert < split) {
                        leaf_split_left(tree, leaf, sibling, key, data, insert);
                } else {
                        leaf_split_right(tree, leaf, sibling, key, data, insert);
                }
                /* build new parent */
                if (insert < split) {
                        return parent_node_build(tree, sibling, leaf, key(leaf)[0], 0);
                } else {
                        return parent_node_build(tree, leaf, sibling, key(sibling)[0], 0);
                }
        } else {
                leaf_simple_insert(tree, leaf, key, data, insert);
        }

        return 0;
}

static int
bplus_tree_insert(struct bplus_tree *tree, int key, long data)
{
        struct bplus_node *node = node_seek(tree, tree->root);
        while (node != NULL) {
                if (is_leaf(node)) {
                        return leaf_insert(tree, node, key, data);
                } else {
                        int i = key_binary_search(node, key);
                        if (i >= 0) {
                                node = node_seek(tree, sub(node)[i + 1]);
                        } else {
                                i = -i - 1;
                                node = node_seek(tree, sub(node)[i]);
                        }
                }
        }

        /* new root */
        struct bplus_node *root = leaf_new(tree);
        key(root)[0] = key;
        data(root)[0] = data;
        root->count = 1;
        tree->root = new_node_write(tree, root);
        node_flush(tree, root);
        return 0;
}

static void
non_leaf_simple_remove(struct bplus_tree *tree, struct bplus_node *node, int remove)
{
        assert(node->count >= 2);
        for (; remove < node->count - 2; remove++) {
                key(node)[remove] = key(node)[remove + 1];
                sub(node)[remove + 1] = sub(node)[remove + 2];
                sub_node_flush(tree, node, sub(node)[remove + 1], remove);
        }
        node->count--;
}

static void
non_leaf_shift_from_left(struct bplus_tree *tree, struct bplus_node *node,
                        struct bplus_node *left, struct bplus_node *parent,
                        int parent_key_index, int remove)
{
        int i;
        /* node's elements right shift */
        for (i = remove; i > 0; i--) {
                key(node)[i] = key(node)[i - 1];
        }
        for (i = remove + 1; i > 0; i--) {
                sub(node)[i] = sub(node)[i - 1];
                sub_node_flush(tree, node, sub(node)[i], i - 1);
        }
        /* parent key right rotation */
        key(node)[0] = key(parent)[parent_key_index];
        key(parent)[parent_key_index] = key(left)[left->count - 2];
        /* borrow the last sub-node from left sibling */
        sub(node)[0] = sub(left)[left->count - 1];
        sub_node_flush(tree, node, sub(node)[0], -1);
        left->count--;
        /* node, parent and left sibling cache flush */
        node_flush(tree, node);
        node_flush(tree, left);
        node_flush(tree, parent);
}

static void
non_leaf_merge_into_left(struct bplus_tree *tree, struct bplus_node *node,
                        struct bplus_node *left, struct bplus_node *parent,
                        int parent_key_index, int remove)
{
        int i, j;
        /* move parent key down */
        key(left)[left->count - 1] = key(parent)[parent_key_index];
        /* merge into left sibling */
        for (i = left->count, j = 0; j < node->count - 1; j++) {
                if (j != remove) {
                        key(left)[i] = key(node)[j];
                        i++;
                }
        }
        for (i = left->count, j = 0; j < node->count; j++) {
                if (j != remove + 1) {
                        sub(left)[i] = sub(node)[j];
                        sub_node_flush(tree, left, sub(left)[i], i - 1);
                        i++;
                }
        }
        left->count = i;
}

static void
non_leaf_shift_from_right(struct bplus_tree *tree, struct bplus_node *node,
                        struct bplus_node *right, struct bplus_node *parent,
                        int parent_key_index)
{
        int i;
        /* parent key left rotation */
        key(node)[node->count - 1] = key(parent)[parent_key_index];
        key(parent)[parent_key_index] = key(right)[0];
        /* borrow the frist sub-node from right sibling */
        sub(node)[node->count] = sub(right)[0];
        sub_node_flush(tree, node, sub(node)[node->count], node->count - 1);
        node->count++;
        /* left shift in right sibling */
        for (i = 0; i < right->count - 2; i++) {
                key(right)[i] = key(right)[i + 1];
        }
        for (i = 0; i < right->count - 1; i++) {
                sub(right)[i] = sub(right)[i + 1];
                sub_node_flush(tree, right, sub(right)[i], i - 1);
        }
        right->count--;
        /* node, parent and right sibling cache flush */
        node_flush(tree, node);
        node_flush(tree, parent);
        node_flush(tree, right);
}

static void
non_leaf_merge_from_right(struct bplus_tree *tree, struct bplus_node *node,
                        struct bplus_node *right, struct bplus_node *parent,
                        int parent_key_index)
{
        int i, j;
        /* move parent key down */
        key(node)[node->count - 1] = key(parent)[parent_key_index];
        node->count++;
        /* merge from right sibling */
        for (i = node->count - 1, j = 0; j < right->count - 1; i++, j++) {
                key(node)[i] = key(right)[j];
        }
        for (i = node->count - 1, j = 0; j < right->count; i++, j++) {
                sub(node)[i] = sub(right)[j];
                sub_node_flush(tree, node, sub(node)[i], i - 1);
        }
        node->count = i;
}

static void
non_leaf_remove(struct bplus_tree *tree, struct bplus_node *node, int remove)
{
        if (node->count <= (tree->order + 1) / 2) {
                struct bplus_node *l_sib = node_fetch(tree, node->prev);
                struct bplus_node *r_sib = node_fetch(tree, node->next);
                struct bplus_node *parent = node_fetch(tree, node->parent);
                if (parent != NULL) {
                        /* decide which sibling to be borrowed from */
                        int borrow = 0;
                        int i = node->parent_key_idx;
                        if (i == -1) {
                                /* the frist sub-node, no left sibling, choose the right one */
                                borrow = RIGHT_SIBLING;
                        } else if (i == parent->count - 2) {
                                /* the last sub-node, no right sibling, choose the left one */
                                borrow = LEFT_SIBLING;
                        } else {
                                /* if both left and right sibling found, choose the one with more children */
                                borrow = l_sib->count >= r_sib->count ? LEFT_SIBLING : RIGHT_SIBLING;
                        }

                        if (borrow == LEFT_SIBLING) {
                                if (l_sib->count > (tree->order + 1) / 2) {
                                        non_leaf_shift_from_left(tree, node, l_sib, parent, i, remove);
                                } else {
                                        non_leaf_merge_into_left(tree, node, l_sib, parent, i, remove);
                                        /* delete empty node and cache flush */
                                        node_delete(tree, node, l_sib, r_sib);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i);
                                }
                        } else {
                                /* remove at first in case of overflow during merging with sibling */
                                non_leaf_simple_remove(tree, node, remove);
                                /* shift or merge */
                                if (r_sib->count > (tree->order + 1) / 2) {
                                        non_leaf_shift_from_right(tree, node, r_sib, parent, i + 1);
                                } else {
                                        non_leaf_merge_from_right(tree, node, r_sib, parent, i + 1);
                                        /* delete empty right sibling and cache flush */
                                        struct bplus_node *rr = node_fetch(tree, r_sib->next);
                                        node_delete(tree, r_sib, node, rr);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1);
                                }
                        }
                } else {
                        if (node->count == 2) {
                                /* delete old root node */
                                assert(remove == 0);
                                struct bplus_node *root = node_fetch(tree, sub(node)[0]);
                                root->parent = INVALID_OFFSET;
                                root->parent_key_idx = -1;
                                node_flush(tree, root);
                                tree->root = root->self;
                                tree->level--;
                                node_delete(tree, node, l_sib, r_sib);
                        } else {
                                non_leaf_simple_remove(tree, node, remove);
                                /* node cache flush */
                                node_flush(tree, node);
                        }
                }
        } else {
                non_leaf_simple_remove(tree, node, remove);
                /* node cache flush */
                node_flush(tree, node);
        }
}

static void
leaf_simple_remove(struct bplus_tree *tree, struct bplus_node *leaf, int remove)
{
        for (; remove < leaf->count - 1; remove++) {
                key(leaf)[remove] = key(leaf)[remove + 1];
                data(leaf)[remove] = data(leaf)[remove + 1];
        }
        leaf->count--;
}

static void
leaf_shift_from_left(struct bplus_tree *tree, struct bplus_node *leaf,
                struct bplus_node *left, struct bplus_node *parent,
                int parent_key_index, int remove)
{
        /* right shift in leaf node */
        for (; remove > 0; remove--) {
                key(leaf)[remove] = key(leaf)[remove - 1];
                data(leaf)[remove] = data(leaf)[remove - 1];
        }
        /* borrow the last element from left sibling */
        key(leaf)[0] = key(left)[left->count - 1];
        data(leaf)[0] = data(left)[left->count - 1];
        left->count--;
        /* update parent key */
        key(parent)[parent_key_index] = key(leaf)[0];
        /* leaf, parent and left sibling cache flush */
        node_flush(tree, leaf);
        node_flush(tree, left);
        node_flush(tree, parent);
}

static void
leaf_merge_into_left(struct bplus_tree *tree, struct bplus_node *leaf,
                struct bplus_node *left, int parent_key_index, int remove)
{
        int i, j;
        /* merge into left sibling */
        for (i = left->count, j = 0; j < leaf->count; j++) {
                if (j != remove) {
                        key(left)[i] = key(leaf)[j];
                        data(left)[i] = data(leaf)[j];
                        i++;
                }
        }
        left->count = i;
}

static void
leaf_shift_from_right(struct bplus_tree *tree, struct bplus_node *leaf,
                        struct bplus_node *right, struct bplus_node *parent,
                        int parent_key_index)
{
        int i;
        /* borrow the first element from right sibling */
        key(leaf)[leaf->count] = key(right)[0];
        data(leaf)[leaf->count] = data(right)[0];
        leaf->count++;
        /* left shift in right sibling */
        for (i = 0; i < right->count - 1; i++) {
                key(right)[i] = key(right)[i + 1];
                data(right)[i] = data(right)[i + 1];
        }
        right->count--;
        /* update parent key */
        key(parent)[parent_key_index] = key(right)[0];
        /* leaf, parent and left sibling cache flush */
        node_flush(tree, leaf);
        node_flush(tree, right);
        node_flush(tree, parent);
}

static void
leaf_merge_from_right(struct bplus_tree *tree, struct bplus_node *leaf,
                        struct bplus_node *right)
{
        int i, j;
        /* merge from right sibling */
        for (i = leaf->count, j = 0; j < right->count; i++, j++) {
                key(leaf)[i] = key(right)[j];
                data(leaf)[i] = data(right)[j];
        }
        leaf->count = i;
}

static int
leaf_remove(struct bplus_tree *tree, struct bplus_node *leaf, int key)
{
        int remove = key_binary_search(leaf, key);
        if (remove < 0) {
                /* Not exist */
                return -1;
        }

        /* fetch from cache list */
        list_del(&leaf->cache->link);

        if (leaf->count <= (tree->entries + 1) / 2) {
                struct bplus_node *l_sib = node_fetch(tree, leaf->prev);
                struct bplus_node *r_sib = node_fetch(tree, leaf->next);
                struct bplus_node *parent = node_fetch(tree, leaf->parent);
                if (parent != NULL) {
                        /* decide which sibling to be borrowed from */
                        int borrow = 0;
                        int i = leaf->parent_key_idx;
                        if (i == -1) {
                                /* the frist sub-node, no left sibling, choose the right one */
                                borrow = RIGHT_SIBLING;
                        } else if (i == parent->count - 2) {
                                /* the last sub-node, no right sibling, choose the left one */
                                borrow = LEFT_SIBLING;
                        } else {
                                /* if both left and right sibling found, choose the one with more entries */
                                borrow = l_sib->count >= r_sib->count ? LEFT_SIBLING : RIGHT_SIBLING;
                        }

                        if (borrow == LEFT_SIBLING) {
                                if (l_sib->count > (tree->entries + 1) / 2) {
                                        leaf_shift_from_left(tree, leaf, l_sib, parent, i, remove);
                                } else {
                                        leaf_merge_into_left(tree, leaf, l_sib, i, remove);
                                        /* delete empty leaf and cache flush */
                                        node_delete(tree, leaf, l_sib, r_sib);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i);
                                }
                        } else {
                                /* remove at first in case of overflow during merging with sibling */
                                leaf_simple_remove(tree, leaf, remove);
                                /* shift or merge */
                                if (r_sib->count > (tree->entries + 1) / 2) {
                                        leaf_shift_from_right(tree, leaf, r_sib, parent, i + 1);
                                } else {
                                        leaf_merge_from_right(tree, leaf, r_sib);
                                        /* delete empty right sibling and cache flush */
                                        struct bplus_node *rr = node_fetch(tree, r_sib->next);
                                        node_delete(tree, r_sib, leaf, rr);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1);
                                }
                        }
                } else {
                        if (leaf->count == 1) {
                                /* delete the only last node */
                                assert(key == key(leaf)[0]);
                                tree->root = INVALID_OFFSET;
                                node_delete(tree, leaf, l_sib, r_sib);
                        } else {
                                leaf_simple_remove(tree, leaf, remove);
                                /* leaf cache flush */ 
                                node_flush(tree, leaf);
                        }
                }
        } else {
                leaf_simple_remove(tree, leaf, remove);
                /* leaf cache flush */ 
                node_flush(tree, leaf);
        }

        return 0;
}

static int
bplus_tree_delete(struct bplus_tree *tree, int key)
{
        struct bplus_node *node = node_seek(tree, tree->root);
        while (node != NULL) {
                if (is_leaf(node)) {
                        return leaf_remove(tree, node, key);
                } else {
                        int i = key_binary_search(node, key);
                        if (i >= 0) {
                                node = node_seek(tree, sub(node)[i + 1]);
                        } else {
                                i = -i - 1;
                                node = node_seek(tree, sub(node)[i]);
                        }
                }
        }
        return -1;
}

long
bplus_tree_get(struct bplus_tree *tree, int key)
{
        long data = bplus_tree_search(tree, key); 
        if (data) {
                return data;
        } else {
                return -1;
        }
}

int
bplus_tree_put(struct bplus_tree *tree, int key, long data)
{
        if (data) {
                return bplus_tree_insert(tree, key, data);
        } else {
                return bplus_tree_delete(tree, key);
        }
}

long
bplus_tree_get_range(struct bplus_tree *tree, int key1, int key2)
{
        long data = 0;
        int min = key1 <= key2 ? key1 : key2;
        int max = min == key1 ? key2 : key1;
        struct bplus_node *node = node_seek(tree, tree->root);

        while (node != NULL) {
                int i = key_binary_search(node, min);
                if (is_leaf(node)) {
                        if (i < 0) {
                                i = -i - 1;
                                if (i >= node->count) {
                                        node = node_seek(tree, node->next);
                                }
                        }
                        while (node != NULL && key(node)[i] <= max) {
                                data = data(node)[i];
                                if (++i >= node->count) {
                                        node = node_seek(tree, node->next);
                                        i = 0;
                                }
                        }
                        break;
                } else {
                        if (i >= 0) {
                                node = node_seek(tree, sub(node)[i + 1]);
                        } else  {
                                i = -i - 1;
                                node = node_seek(tree, sub(node)[i]);
                        }
                }
        }

        return data;
}

int
bplus_open(char *filename)
{
        return open(filename, O_CREAT | O_RDWR, 0644);
}

void
bplus_close(int fd)
{
        close(fd);
}

struct bplus_tree *
bplus_tree_init(char *filename, int block_size)
{
        int i;
        struct bplus_node node;
        assert((block_size & (block_size - 1)) == 0);
        struct bplus_tree *tree = calloc(1, sizeof(*tree));
        if (tree != NULL) {
                tree->root = INVALID_OFFSET;
                tree->block_size = block_size;
                max_order = tree->order = (block_size - sizeof(node)) / (sizeof(int) + sizeof(off_t));
                max_entries = tree->entries = (block_size - sizeof(node)) / (sizeof(int) + sizeof(long));
                if (tree->order <= 2) {
                        fprintf(stderr, "block size is too small for one node!\n");
                        exit(-1);
                }
                printf("config node order:%d and leaf entries:%d\n", tree->order, tree->entries);
                list_init(&tree->free_blocks);
                list_init(&tree->free_caches);
                for (i = 0; i < 100000; i++) {
                        struct free_cache *cache = calloc(1, sizeof(*cache));
                        assert(cache != NULL);
                        cache->buf = malloc(tree->block_size);
                        assert(cache->buf != NULL);
                        list_add(&cache->link, &tree->free_caches);
                }
                tree->fd = bplus_open(filename);
                assert(tree->fd >= 0);
        }
        return tree;
}

void
bplus_tree_deinit(struct bplus_tree *tree)
{
        struct list_head *pos, *n;
        /* In fact these free blocks need to be recorded at some place... */
        list_for_each_safe(pos, n, &tree->free_blocks) {
                list_del(pos);
                free(list_entry(pos, struct free_block, link));
        }
        bplus_close(tree->fd);
        free(tree);
}

#ifdef _BPLUS_TREE_DEBUG

#define MAX_LEVEL 64

typedef struct node_backlog {
        /* Node backlogged */
        off_t offset;
        /* The index next to the backtrack point, must be >= 1 */
        int next_sub_idx;
} node_backlog;

static inline void
nbl_push(struct node_backlog *nbl, struct node_backlog **top, struct node_backlog **buttom)
{
        if (*top - *buttom < MAX_LEVEL) {
                (*(*top)++) = *nbl;
        }
}

static inline struct node_backlog *
nbl_pop(struct node_backlog **top, struct node_backlog **buttom)
{
        return *top - *buttom > 0 ? --*top : NULL;
}

static inline int
children(struct bplus_node *node)
{
        assert(!is_leaf(node));
        return node->count;
}
static void
node_key_dump(struct bplus_node *node)
{
        int i;
        if (is_leaf(node)) {
                printf("leaf:");
                for (i = 0; i < node->count; i++) {
                        printf(" %d", key(node)[i]);
                }
        } else {
                printf("node:");
                for (i = 0; i < node->count - 1; i++) {
                        printf(" %d", key(node)[i]);
                }
        }
        printf("\n");
}

static void
draw(struct bplus_tree *tree, struct bplus_node *node, struct node_backlog *stack, int level)
{
        int i;
        for (i = 1; i < level; i++) {
                if (i == level - 1) {
                        printf("%-8s", "+-------");
                } else {
                        if (stack[i - 1].offset != INVALID_OFFSET) {
                                printf("%-8s", "|");
                        } else {
                                printf("%-8s", " ");
                        }
                }
        }
        node_key_dump(node);
}

void
bplus_tree_dump(struct bplus_tree *tree)
{
        int level = 0;
        struct bplus_node *node = node_seek(tree, tree->root);
        struct node_backlog nbl, *p_nbl = NULL;
        struct node_backlog *top, *buttom, nbl_stack[MAX_LEVEL];

        top = buttom = nbl_stack;

        for (; ;) {
                if (node != NULL) {
                        /* Fetch the pop-up backlogged node's sub-id.
                         * If not backlogged, fetch the first sub-id. */
                        int sub_idx = p_nbl != NULL ? p_nbl->next_sub_idx : 0;
                        /* Reset backlog for the node has gone deep down */
                        p_nbl = NULL;

                        /* Backlog the node */
                        if (is_leaf(node) || sub_idx + 1 >= children(node)) {
                                nbl.offset = INVALID_OFFSET;
                                nbl.next_sub_idx = 0;
                        } else {
                                nbl.offset = node->self;
                                nbl.next_sub_idx = sub_idx + 1;
                        }
                        nbl_push(&nbl, &top, &buttom);
                        level++;

                        /* Draw lines as long as sub_idx is the first one */
                        if (sub_idx == 0) {
                                draw(tree, node, nbl_stack, level);
                        }

                        /* Move deep down */
                        node = is_leaf(node) ? NULL : node_seek(tree, sub(node)[sub_idx]);
                } else {
                        p_nbl = nbl_pop(&top, &buttom);
                        if (p_nbl == NULL) {
                                /* End of traversal */
                                break;
                        }
                        node = node_seek(tree, p_nbl->offset);
                        level--;
                }
        }
}

#endif
