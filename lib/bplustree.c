/*
 * Copyright (C) 2017, Leo Ma <begeekmyfriend@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bplustree.h"

enum {
        INVALID_OFFSET = 0xdeadbeef,
};

enum {
        BPLUS_TREE_LEAF,
        BPLUS_TREE_NON_LEAF = 1,
};

enum {
        LEFT_SIBLING,
        RIGHT_SIBLING = 1,
};

#define ADDR_STR_WIDTH 16
#define offset_ptr(node) ((char *) (node) + sizeof(*node))
#define key(node) ((key_t *)offset_ptr(node))
#define data(node) ((long *)(offset_ptr(node) + _max_entries * sizeof(key_t)))
#define sub(node) ((off_t *)(offset_ptr(node) + (_max_order - 1) * sizeof(key_t)))

static int _block_size;
static int _max_entries;
static int _max_order;

static inline int is_leaf(struct bplus_node *node)
{
        return node->type == BPLUS_TREE_LEAF;
}

static int key_binary_search(struct bplus_node *node, key_t target)
{
        key_t *arr = key(node);
        int len = is_leaf(node) ? node->children : node->children - 1;
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

static inline int parent_key_index(struct bplus_node *parent, key_t key)
{
        int index = key_binary_search(parent, key);
        return index >= 0 ? index : -index - 2;
}

static inline struct bplus_node *cache_refer(struct bplus_tree *tree)
{
        int i;
        for (i = 0; i < MIN_CACHE_NUM; i++) {
                if (!tree->used[i]) {
                        tree->used[i] = 1;
                        char *buf = tree->caches + _block_size * i;
                        return (struct bplus_node *) buf;
                }
        }
        assert(0);
}

static inline void cache_defer(struct bplus_tree *tree, struct bplus_node *node)
{
        /* return the node cache borrowed from */
        char *buf = (char *) node;
        int i = (buf - tree->caches) / _block_size;
        tree->used[i] = 0;
}

static struct bplus_node *node_new(struct bplus_tree *tree)
{
        struct bplus_node *node = cache_refer(tree);
        node->self = INVALID_OFFSET;
        node->parent = INVALID_OFFSET;
        node->prev = INVALID_OFFSET;
        node->next = INVALID_OFFSET;
        node->children = 0;
        return node;
}

static inline struct bplus_node *non_leaf_new(struct bplus_tree *tree)
{
        struct bplus_node *node = node_new(tree);
        node->type = BPLUS_TREE_NON_LEAF;
        return node;
}

static inline struct bplus_node *leaf_new(struct bplus_tree *tree)
{
        struct bplus_node *node = node_new(tree);
        node->type = BPLUS_TREE_LEAF;
        return node;
}

static struct bplus_node *node_fetch(struct bplus_tree *tree, off_t offset)
{
        if (offset == INVALID_OFFSET) {
                return NULL;
        }

        struct bplus_node *node = cache_refer(tree);
        int len = pread(tree->fd, node, _block_size, offset);
        assert(len == _block_size);
        return node;
}

static struct bplus_node *node_seek(struct bplus_tree *tree, off_t offset)
{
        if (offset == INVALID_OFFSET) {
                return NULL;
        }

        int i;
        for (i = 0; i < MIN_CACHE_NUM; i++) {
                if (!tree->used[i]) {
                        char *buf = tree->caches + _block_size * i;
                        int len = pread(tree->fd, buf, _block_size, offset);
                        assert(len == _block_size);
                        return (struct bplus_node *) buf;
                }
        }
        assert(0);
}

static inline void node_flush(struct bplus_tree *tree, struct bplus_node *node)
{
        if (node != NULL) {
                int len = pwrite(tree->fd, node, _block_size, node->self);
                assert(len == _block_size);
                cache_defer(tree, node);
        }
}

static off_t new_node_append(struct bplus_tree *tree, struct bplus_node *node)
{
        /* assign new offset to the new node */
        if (list_empty(&tree->free_blocks)) {
                node->self = tree->file_size;
                tree->file_size += _block_size;
        } else {
                struct free_block *block;
                block = list_first_entry(&tree->free_blocks, struct free_block, link);
                list_del(&block->link);
                node->self = block->offset;
                free(block);
        }
        return node->self;
}

static void node_delete(struct bplus_tree *tree, struct bplus_node *node,
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
        /* deleted blocks can be allocated for other nodes */
        block->offset = node->self;
        list_add_tail(&block->link, &tree->free_blocks);
        /* return the node cache borrowed from */
        cache_defer(tree, node);
}

static inline void sub_node_update(struct bplus_tree *tree, struct bplus_node *parent,
                		   int index, struct bplus_node *sub_node)
{
        assert(sub_node->self != INVALID_OFFSET);
        sub(parent)[index] = sub_node->self;
        sub_node->parent = parent->self;
        node_flush(tree, sub_node);
}

static inline void sub_node_flush(struct bplus_tree *tree, struct bplus_node *parent, off_t sub_offset)
{
        struct bplus_node *sub_node = node_fetch(tree, sub_offset);
        assert(sub_node != NULL);
        sub_node->parent = parent->self;
        node_flush(tree, sub_node);
}

static long bplus_tree_search(struct bplus_tree *tree, key_t key)
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

static void left_node_add(struct bplus_tree *tree, struct bplus_node *node, struct bplus_node *left)
{
        new_node_append(tree, left);

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

static void right_node_add(struct bplus_tree *tree, struct bplus_node *node, struct bplus_node *right)
{
        new_node_append(tree, right);

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

static key_t non_leaf_insert(struct bplus_tree *tree, struct bplus_node *node,
                             struct bplus_node *l_ch, struct bplus_node *r_ch, key_t key);

static int parent_node_build(struct bplus_tree *tree, struct bplus_node *l_ch,
                             struct bplus_node *r_ch, key_t key)
{
        if (l_ch->parent == INVALID_OFFSET && r_ch->parent == INVALID_OFFSET) {
                /* new parent */
                struct bplus_node *parent = non_leaf_new(tree);
                key(parent)[0] = key;
                sub(parent)[0] = l_ch->self;
                sub(parent)[1] = r_ch->self;
                parent->children = 2;
                /* write new parent and update root */
                tree->root = new_node_append(tree, parent);
                l_ch->parent = parent->self;
                r_ch->parent = parent->self;
                tree->level++;
                /* flush parent, left and right child */
                node_flush(tree, l_ch);
                node_flush(tree, r_ch);
                node_flush(tree, parent);
                return 0;
        } else if (r_ch->parent == INVALID_OFFSET) {
                return non_leaf_insert(tree, node_fetch(tree, l_ch->parent), l_ch, r_ch, key);
        } else {
                return non_leaf_insert(tree, node_fetch(tree, r_ch->parent), l_ch, r_ch, key);
        }
}

static key_t non_leaf_split_left(struct bplus_tree *tree, struct bplus_node *node,
                	         struct bplus_node *left, struct bplus_node *l_ch,
                	         struct bplus_node *r_ch, key_t key, int insert)
{
        int i;
        key_t split_key;

        /* split = [m/2] */
        int split = (_max_order + 1) / 2;

        /* split as left sibling */
        left_node_add(tree, node, left);

        /* calculate split nodes' children (sum as (order + 1))*/
        int pivot = insert;
        left->children = split;
        node->children = _max_order - split + 1;

        /* sum = left->children = pivot + (split - pivot - 1) + 1 */
        /* replicate from key[0] to key[insert] in original node */
        memmove(&key(left)[0], &key(node)[0], pivot * sizeof(key_t));
        memmove(&sub(left)[0], &sub(node)[0], pivot * sizeof(off_t));

        /* replicate from key[insert] to key[split - 1] in original node */
        memmove(&key(left)[pivot + 1], &key(node)[pivot], (split - pivot - 1) * sizeof(key_t));
        memmove(&sub(left)[pivot + 1], &sub(node)[pivot], (split - pivot - 1) * sizeof(off_t));

        /* flush sub-nodes of the new splitted left node */
        for (i = 0; i < left->children; i++) {
                if (i != pivot && i != pivot + 1) {
                        sub_node_flush(tree, left, sub(left)[i]);
                }
        }

        /* insert new key and sub-nodes and locate the split key */
        key(left)[pivot] = key;
        if (pivot == split - 1) {
                /* left child in split left node and right child in original right one */
                sub_node_update(tree, left, pivot, l_ch);
                sub_node_update(tree, node, 0, r_ch);
                split_key = key;
        } else {
                /* both new children in split left node */
                sub_node_update(tree, left, pivot, l_ch);
                sub_node_update(tree, left, pivot + 1, r_ch);
                sub(node)[0] = sub(node)[split - 1];
                split_key = key(node)[split - 2];
        }

        /* sum = node->children = 1 + (node->children - 1) */
        /* right node left shift from key[split - 1] to key[children - 2] */
        memmove(&key(node)[0], &key(node)[split - 1], (node->children - 1) * sizeof(key_t));
        memmove(&sub(node)[1], &sub(node)[split], (node->children - 1) * sizeof(off_t));

        return split_key;
}

static key_t non_leaf_split_right1(struct bplus_tree *tree, struct bplus_node *node,
                        	   struct bplus_node *right, struct bplus_node *l_ch,
                        	   struct bplus_node *r_ch, key_t key, int insert)
{
        int i;

        /* split = [m/2] */
        int split = (_max_order + 1) / 2;

        /* split as right sibling */
        right_node_add(tree, node, right);

        /* split key is key[split - 1] */
        key_t split_key = key(node)[split - 1];

        /* calculate split nodes' children (sum as (order + 1))*/
        int pivot = 0;
        node->children = split;
        right->children = _max_order - split + 1;

        /* insert new key and sub-nodes */
        key(right)[0] = key;
        sub_node_update(tree, right, pivot, l_ch);
        sub_node_update(tree, right, pivot + 1, r_ch);

        /* sum = right->children = 2 + (right->children - 2) */
        /* replicate from key[split] to key[_max_order - 2] */
        memmove(&key(right)[pivot + 1], &key(node)[split], (right->children - 2) * sizeof(key_t));
        memmove(&sub(right)[pivot + 2], &sub(node)[split + 1], (right->children - 2) * sizeof(off_t));

        /* flush sub-nodes of the new splitted right node */
        for (i = pivot + 2; i < right->children; i++) {
                sub_node_flush(tree, right, sub(right)[i]);
        }

        return split_key;
}

static key_t non_leaf_split_right2(struct bplus_tree *tree, struct bplus_node *node,
                        	   struct bplus_node *right, struct bplus_node *l_ch,
                        	   struct bplus_node *r_ch, key_t key, int insert)
{
        int i;

        /* split = [m/2] */
        int split = (_max_order + 1) / 2;

        /* split as right sibling */
        right_node_add(tree, node, right);

        /* split key is key[split] */
        key_t split_key = key(node)[split];

        /* calculate split nodes' children (sum as (order + 1))*/
        int pivot = insert - split - 1;
        node->children = split + 1;
        right->children = _max_order - split;

        /* sum = right->children = pivot + 2 + (_max_order - insert - 1) */
        /* replicate from key[split + 1] to key[insert] */
        memmove(&key(right)[0], &key(node)[split + 1], pivot * sizeof(key_t));
        memmove(&sub(right)[0], &sub(node)[split + 1], pivot * sizeof(off_t));

        /* insert new key and sub-node */
        key(right)[pivot] = key;
        sub_node_update(tree, right, pivot, l_ch);
        sub_node_update(tree, right, pivot + 1, r_ch);

        /* replicate from key[insert] to key[order - 1] */
        memmove(&key(right)[pivot + 1], &key(node)[insert], (_max_order - insert - 1) * sizeof(key_t));
        memmove(&sub(right)[pivot + 2], &sub(node)[insert + 1], (_max_order - insert - 1) * sizeof(off_t));

        /* flush sub-nodes of the new splitted right node */
        for (i = 0; i < right->children; i++) {
                if (i != pivot && i != pivot + 1) {
                        sub_node_flush(tree, right, sub(right)[i]);
                }
        }

        return split_key;
}

static void non_leaf_simple_insert(struct bplus_tree *tree, struct bplus_node *node,
                        	   struct bplus_node *l_ch, struct bplus_node *r_ch,
                        	   key_t key, int insert)
{
        memmove(&key(node)[insert + 1], &key(node)[insert], (node->children - 1 - insert) * sizeof(key_t));
        memmove(&sub(node)[insert + 2], &sub(node)[insert + 1], (node->children - 1 - insert) * sizeof(off_t));
        /* insert new key and sub-nodes */
        key(node)[insert] = key;
        sub_node_update(tree, node, insert, l_ch);
        sub_node_update(tree, node, insert + 1, r_ch);
        node->children++;
}

static int non_leaf_insert(struct bplus_tree *tree, struct bplus_node *node,
                	   struct bplus_node *l_ch, struct bplus_node *r_ch, key_t key)
{
        /* Search key location */
        int insert = key_binary_search(node, key);
        assert(insert < 0);
        insert = -insert - 1;

        /* node is full */
        if (node->children == _max_order) {
                key_t split_key;
                /* split = [m/2] */
                int split = (node->children + 1) / 2;
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
                        return parent_node_build(tree, sibling, node, split_key);
                } else {
                        return parent_node_build(tree, node, sibling, split_key);
                }
        } else {
                non_leaf_simple_insert(tree, node, l_ch, r_ch, key, insert);
                node_flush(tree, node);
        }
        return 0;
}

static key_t leaf_split_left(struct bplus_tree *tree, struct bplus_node *leaf,
                	     struct bplus_node *left, key_t key, long data, int insert)
{
        /* split = [m/2] */
        int split = (leaf->children + 1) / 2;

        /* split as left sibling */
        left_node_add(tree, leaf, left);

        /* calculate split leaves' children (sum as (entries + 1)) */
        int pivot = insert;
        left->children = split;
        leaf->children = _max_entries - split + 1;

        /* sum = left->children = pivot + 1 + (split - pivot - 1) */
        /* replicate from key[0] to key[insert] */
        memmove(&key(left)[0], &key(leaf)[0], pivot * sizeof(key_t));
        memmove(&data(left)[0], &data(leaf)[0], pivot * sizeof(long));

        /* insert new key and data */
        key(left)[pivot] = key;
        data(left)[pivot] = data;

        /* replicate from key[insert] to key[split - 1] */
        memmove(&key(left)[pivot + 1], &key(leaf)[pivot], (split - pivot - 1) * sizeof(key_t));
        memmove(&data(left)[pivot + 1], &data(leaf)[pivot], (split - pivot - 1) * sizeof(long));

        /* original leaf left shift */
        memmove(&key(leaf)[0], &key(leaf)[split - 1], leaf->children * sizeof(key_t));
        memmove(&data(leaf)[0], &data(leaf)[split - 1], leaf->children * sizeof(long));

        return key(leaf)[0];
}

static key_t leaf_split_right(struct bplus_tree *tree, struct bplus_node *leaf,
                	      struct bplus_node *right, key_t key, long data, int insert)
{
        /* split = [m/2] */
        int split = (leaf->children + 1) / 2;

        /* split as right sibling */
        right_node_add(tree, leaf, right);

        /* calculate split leaves' children (sum as (entries + 1)) */
        int pivot = insert - split;
        leaf->children = split;
        right->children = _max_entries - split + 1;

        /* sum = right->children = pivot + 1 + (_max_entries - pivot - split) */
        /* replicate from key[split] to key[children - 1] in original leaf */
        memmove(&key(right)[0], &key(leaf)[split], pivot * sizeof(key_t));
        memmove(&data(right)[0], &data(leaf)[split], pivot * sizeof(long));

        /* insert new key and data */
        key(right)[pivot] = key;
        data(right)[pivot] = data;

        /* replicate from key[insert] to key[children - 1] in original leaf */
        memmove(&key(right)[pivot + 1], &key(leaf)[insert], (_max_entries - insert) * sizeof(key_t));
        memmove(&data(right)[pivot + 1], &data(leaf)[insert], (_max_entries - insert) * sizeof(long));

        return key(right)[0];
}

static void leaf_simple_insert(struct bplus_tree *tree, struct bplus_node *leaf,
                	       key_t key, long data, int insert)
{
        memmove(&key(leaf)[insert + 1], &key(leaf)[insert], (leaf->children - insert) * sizeof(key_t));
        memmove(&data(leaf)[insert + 1], &data(leaf)[insert], (leaf->children - insert) * sizeof(long));
        key(leaf)[insert] = key;
        data(leaf)[insert] = data;
        leaf->children++;
}

static int leaf_insert(struct bplus_tree *tree, struct bplus_node *leaf, key_t key, long data)
{
        /* Search key location */
        int insert = key_binary_search(leaf, key);
        if (insert >= 0) {
                /* Already exists */
                return -1;
        }
        insert = -insert - 1;

        /* fetch from free node caches */
        int i = ((char *) leaf - tree->caches) / _block_size;
        tree->used[i] = 1;

        /* leaf is full */
        if (leaf->children == _max_entries) {
                key_t split_key;
                /* split = [m/2] */
                int split = (_max_entries + 1) / 2;
                struct bplus_node *sibling = leaf_new(tree);

                /* sibling leaf replication due to location of insertion */
                if (insert < split) {
                        split_key = leaf_split_left(tree, leaf, sibling, key, data, insert);
                } else {
                        split_key = leaf_split_right(tree, leaf, sibling, key, data, insert);
                }

                /* build new parent */
                if (insert < split) {
                        return parent_node_build(tree, sibling, leaf, split_key);
                } else {
                        return parent_node_build(tree, leaf, sibling, split_key);
                }
        } else {
                leaf_simple_insert(tree, leaf, key, data, insert);
                node_flush(tree, leaf);
        }

        return 0;
}

static int bplus_tree_insert(struct bplus_tree *tree, key_t key, long data)
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
        root->children = 1;
        tree->root = new_node_append(tree, root);
        tree->level = 1;
        node_flush(tree, root);
        return 0;
}

static inline int sibling_select(struct bplus_node *l_sib, struct bplus_node *r_sib,
                                 struct bplus_node *parent, int i)
{
        if (i == -1) {
                /* the frist sub-node, no left sibling, choose the right one */
                return RIGHT_SIBLING;
        } else if (i == parent->children - 2) {
                /* the last sub-node, no right sibling, choose the left one */
                return LEFT_SIBLING;
        } else {
                /* if both left and right sibling found, choose the one with more children */
                return l_sib->children >= r_sib->children ? LEFT_SIBLING : RIGHT_SIBLING;
        }
}

static void non_leaf_shift_from_left(struct bplus_tree *tree, struct bplus_node *node,
                        	     struct bplus_node *left, struct bplus_node *parent,
                        	     int parent_key_index, int remove)
{
        /* node's elements right shift */
        memmove(&key(node)[1], &key(node)[0], remove * sizeof(key_t));
        memmove(&sub(node)[1], &sub(node)[0], (remove + 1) * sizeof(off_t));

        /* parent key right rotation */
        key(node)[0] = key(parent)[parent_key_index];
        key(parent)[parent_key_index] = key(left)[left->children - 2];

        /* borrow the last sub-node from left sibling */
        sub(node)[0] = sub(left)[left->children - 1];
        sub_node_flush(tree, node, sub(node)[0]);

        left->children--;
}

static void non_leaf_merge_into_left(struct bplus_tree *tree, struct bplus_node *node,
                        	     struct bplus_node *left, struct bplus_node *parent,
                        	     int parent_key_index, int remove)
{
        /* move parent key down */
        key(left)[left->children - 1] = key(parent)[parent_key_index];

        /* merge into left sibling */
        /* key sum = node->children - 2 */
        memmove(&key(left)[left->children], &key(node)[0], remove * sizeof(key_t));
        memmove(&sub(left)[left->children], &sub(node)[0], (remove + 1) * sizeof(off_t));

        /* sub-node sum = node->children - 1 */
        memmove(&key(left)[left->children + remove], &key(node)[remove + 1], (node->children - remove - 2) * sizeof(key_t));
        memmove(&sub(left)[left->children + remove + 1], &sub(node)[remove + 2], (node->children - remove - 2) * sizeof(off_t));

        /* flush sub-nodes of the new merged left node */
        int i, j;
        for (i = left->children, j = 0; j < node->children - 1; i++, j++) {
                sub_node_flush(tree, left, sub(left)[i]);
        }

        left->children += node->children - 1;
}

static void non_leaf_shift_from_right(struct bplus_tree *tree, struct bplus_node *node,
                        	      struct bplus_node *right, struct bplus_node *parent,
                        	      int parent_key_index)
{
        /* parent key left rotation */
        key(node)[node->children - 1] = key(parent)[parent_key_index];
        key(parent)[parent_key_index] = key(right)[0];

        /* borrow the frist sub-node from right sibling */
        sub(node)[node->children] = sub(right)[0];
        sub_node_flush(tree, node, sub(node)[node->children]);
        node->children++;

        /* right sibling left shift*/
        memmove(&key(right)[0], &key(right)[1], (right->children - 2) * sizeof(key_t));
        memmove(&sub(right)[0], &sub(right)[1], (right->children - 1) * sizeof(off_t));

        right->children--;
}

static void non_leaf_merge_from_right(struct bplus_tree *tree, struct bplus_node *node,
                        	      struct bplus_node *right, struct bplus_node *parent,
                        	      int parent_key_index)
{
        /* move parent key down */
        key(node)[node->children - 1] = key(parent)[parent_key_index];
        node->children++;

        /* merge from right sibling */
        memmove(&key(node)[node->children - 1], &key(right)[0], (right->children - 1) * sizeof(key_t));
        memmove(&sub(node)[node->children - 1], &sub(right)[0], right->children * sizeof(off_t));

        /* flush sub-nodes of the new merged node */
        int i, j;
        for (i = node->children - 1, j = 0; j < right->children; i++, j++) {
                sub_node_flush(tree, node, sub(node)[i]);
        }

        node->children += right->children - 1;
}

static inline void non_leaf_simple_remove(struct bplus_tree *tree, struct bplus_node *node, int remove)
{
        assert(node->children >= 2);
        memmove(&key(node)[remove], &key(node)[remove + 1], (node->children - remove - 2) * sizeof(key_t));
        memmove(&sub(node)[remove + 1], &sub(node)[remove + 2], (node->children - remove - 2) * sizeof(off_t));
        node->children--;
}

static void non_leaf_remove(struct bplus_tree *tree, struct bplus_node *node, int remove)
{
        if (node->parent == INVALID_OFFSET) {
                /* node is the root */
                if (node->children == 2) {
                        /* replace old root with the first sub-node */
                        struct bplus_node *root = node_fetch(tree, sub(node)[0]);
                        root->parent = INVALID_OFFSET;
                        tree->root = root->self;
                        tree->level--;
                        node_delete(tree, node, NULL, NULL);
                        node_flush(tree, root);
                } else {
                        non_leaf_simple_remove(tree, node, remove);
                        node_flush(tree, node);
                }
        } else if (node->children <= (_max_order + 1) / 2) {
                struct bplus_node *l_sib = node_fetch(tree, node->prev);
                struct bplus_node *r_sib = node_fetch(tree, node->next);
                struct bplus_node *parent = node_fetch(tree, node->parent);

                int i = parent_key_index(parent, key(node)[0]);

                /* decide which sibling to be borrowed from */
                if (sibling_select(l_sib, r_sib, parent, i)  == LEFT_SIBLING) {
                        if (l_sib->children > (_max_order + 1) / 2) {
                                non_leaf_shift_from_left(tree, node, l_sib, parent, i, remove);
                                /* flush nodes */
                                node_flush(tree, node);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                non_leaf_merge_into_left(tree, node, l_sib, parent, i, remove);
                                /* delete empty node and flush */
                                node_delete(tree, node, l_sib, r_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i);
                        }
                } else {
                        /* remove at first in case of overflow during merging with sibling */
                        non_leaf_simple_remove(tree, node, remove);

                        if (r_sib->children > (_max_order + 1) / 2) {
                                non_leaf_shift_from_right(tree, node, r_sib, parent, i + 1);
                                /* flush nodes */
                                node_flush(tree, node);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                non_leaf_merge_from_right(tree, node, r_sib, parent, i + 1);
                                /* delete empty right sibling and flush */
                                struct bplus_node *rr_sib = node_fetch(tree, r_sib->next);
                                node_delete(tree, r_sib, node, rr_sib);
                                node_flush(tree, l_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i + 1);
                        }
                }
        } else {
                non_leaf_simple_remove(tree, node, remove);
                node_flush(tree, node);
        }
}

static void leaf_shift_from_left(struct bplus_tree *tree, struct bplus_node *leaf,
                		 struct bplus_node *left, struct bplus_node *parent,
                		 int parent_key_index, int remove)
{
        /* right shift in leaf node */
        memmove(&key(leaf)[1], &key(leaf)[0], remove * sizeof(key_t));
        memmove(&data(leaf)[1], &data(leaf)[0], remove * sizeof(off_t));

        /* borrow the last element from left sibling */
        key(leaf)[0] = key(left)[left->children - 1];
        data(leaf)[0] = data(left)[left->children - 1];
        left->children--;

        /* update parent key */
        key(parent)[parent_key_index] = key(leaf)[0];
}

static void leaf_merge_into_left(struct bplus_tree *tree, struct bplus_node *leaf,
                		 struct bplus_node *left, int parent_key_index, int remove)
{
        /* merge into left sibling, sum = leaf->children - 1*/
        memmove(&key(left)[left->children], &key(leaf)[0], remove * sizeof(key_t));
        memmove(&data(left)[left->children], &data(leaf)[0], remove * sizeof(off_t));
        memmove(&key(left)[left->children + remove], &key(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(key_t));
        memmove(&data(left)[left->children + remove], &data(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(off_t));
        left->children += leaf->children - 1;
}

static void leaf_shift_from_right(struct bplus_tree *tree, struct bplus_node *leaf,
                                  struct bplus_node *right, struct bplus_node *parent,
                                  int parent_key_index)
{
        /* borrow the first element from right sibling */
        key(leaf)[leaf->children] = key(right)[0];
        data(leaf)[leaf->children] = data(right)[0];
        leaf->children++;

        /* left shift in right sibling */
        memmove(&key(right)[0], &key(right)[1], (right->children - 1) * sizeof(key_t));
        memmove(&data(right)[0], &data(right)[1], (right->children - 1) * sizeof(off_t));
        right->children--;

        /* update parent key */
        key(parent)[parent_key_index] = key(right)[0];
}

static inline void leaf_merge_from_right(struct bplus_tree *tree, struct bplus_node *leaf,
                                         struct bplus_node *right)
{
        memmove(&key(leaf)[leaf->children], &key(right)[0], right->children * sizeof(key_t));
        memmove(&data(leaf)[leaf->children], &data(right)[0], right->children * sizeof(off_t));
        leaf->children += right->children;
}

static inline void leaf_simple_remove(struct bplus_tree *tree, struct bplus_node *leaf, int remove)
{
        memmove(&key(leaf)[remove], &key(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(key_t));
        memmove(&data(leaf)[remove], &data(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(off_t));
        leaf->children--;
}

static int leaf_remove(struct bplus_tree *tree, struct bplus_node *leaf, key_t key)
{
        int remove = key_binary_search(leaf, key);
        if (remove < 0) {
                /* Not exist */
                return -1;
        }

        /* fetch from free node caches */
        int i = ((char *) leaf - tree->caches) / _block_size;
        tree->used[i] = 1;

        if (leaf->parent == INVALID_OFFSET) {
                /* leaf as the root */
                if (leaf->children == 1) {
                        /* delete the only last node */
                        assert(key == key(leaf)[0]);
                        tree->root = INVALID_OFFSET;
                        tree->level = 0;
                        node_delete(tree, leaf, NULL, NULL);
                } else {
                        leaf_simple_remove(tree, leaf, remove);
                        node_flush(tree, leaf);
                }
        } else if (leaf->children <= (_max_entries + 1) / 2) {
                struct bplus_node *l_sib = node_fetch(tree, leaf->prev);
                struct bplus_node *r_sib = node_fetch(tree, leaf->next);
                struct bplus_node *parent = node_fetch(tree, leaf->parent);

                i = parent_key_index(parent, key(leaf)[0]);

                /* decide which sibling to be borrowed from */
                if (sibling_select(l_sib, r_sib, parent, i) == LEFT_SIBLING) {
                        if (l_sib->children > (_max_entries + 1) / 2) {
                                leaf_shift_from_left(tree, leaf, l_sib, parent, i, remove);
                                /* flush leaves */
                                node_flush(tree, leaf);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                leaf_merge_into_left(tree, leaf, l_sib, i, remove);
                                /* delete empty leaf and flush */
                                node_delete(tree, leaf, l_sib, r_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i);
                        }
                } else {
                        /* remove at first in case of overflow during merging with sibling */
                        leaf_simple_remove(tree, leaf, remove);

                        if (r_sib->children > (_max_entries + 1) / 2) {
                                leaf_shift_from_right(tree, leaf, r_sib, parent, i + 1);
                                /* flush leaves */
                                node_flush(tree, leaf);
                                node_flush(tree, l_sib);
                                node_flush(tree, r_sib);
                                node_flush(tree, parent);
                        } else {
                                leaf_merge_from_right(tree, leaf, r_sib);
                                /* delete empty right sibling flush */
                                struct bplus_node *rr_sib = node_fetch(tree, r_sib->next);
                                node_delete(tree, r_sib, leaf, rr_sib);
                                node_flush(tree, l_sib);
                                /* trace upwards */
                                non_leaf_remove(tree, parent, i + 1);
                        }
                }
        } else {
                leaf_simple_remove(tree, leaf, remove);
                node_flush(tree, leaf);
        }

        return 0;
}

static int bplus_tree_delete(struct bplus_tree *tree, key_t key)
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

long bplus_tree_get(struct bplus_tree *tree, key_t key)
{
        return bplus_tree_search(tree, key);
}

int bplus_tree_put(struct bplus_tree *tree, key_t key, long data)
{
        if (data) {
                return bplus_tree_insert(tree, key, data);
        } else {
                return bplus_tree_delete(tree, key);
        }
}

long bplus_tree_get_range(struct bplus_tree *tree, key_t key1, key_t key2)
{
        long start = -1;
        key_t min = key1 <= key2 ? key1 : key2;
        key_t max = min == key1 ? key2 : key1;

        struct bplus_node *node = node_seek(tree, tree->root);
        while (node != NULL) {
                int i = key_binary_search(node, min);
                if (is_leaf(node)) {
                        if (i < 0) {
                                i = -i - 1;
                                if (i >= node->children) {
                                        node = node_seek(tree, node->next);
                                }
                        }
                        while (node != NULL && key(node)[i] <= max) {
                                start = data(node)[i];
                                if (++i >= node->children) {
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

        return start;
}

int bplus_open(char *filename)
{
        return open(filename, O_CREAT | O_RDWR, 0644);
}

void bplus_close(int fd)
{
        close(fd);
}

static off_t str_to_hex(char *c, int len)
{
        off_t offset = 0;
        while (len-- > 0) {
                if (isdigit(*c)) {
                        offset = offset * 16 + *c - '0';
                } else if (isxdigit(*c)) {
                        if (islower(*c)) {
                                offset = offset * 16 + *c - 'a' + 10;
                        } else {
                                offset = offset * 16 + *c - 'A' + 10;
                        }
                }
                c++;
        }
        return offset;
}

static inline void hex_to_str(off_t offset, char *buf, int len)
{
        const static char *hex = "0123456789ABCDEF";
        while (len-- > 0) {
                buf[len] = hex[offset & 0xf];
                offset >>= 4;
        }
}

static inline off_t offset_load(int fd)
{
        char buf[ADDR_STR_WIDTH];
        ssize_t len = read(fd, buf, sizeof(buf));
        return len > 0 ? str_to_hex(buf, sizeof(buf)) : INVALID_OFFSET;
}

static inline ssize_t offset_store(int fd, off_t offset)
{
        char buf[ADDR_STR_WIDTH];
        hex_to_str(offset, buf, sizeof(buf));
        return write(fd, buf, sizeof(buf));
}

struct bplus_tree *bplus_tree_init(char *filename, int block_size)
{
        int i;
        struct bplus_node node;

        if (strlen(filename) >= 1024) {
                fprintf(stderr, "Index file name too long!\n");
                return NULL;
        }

        if ((block_size & (block_size - 1)) != 0) {
                fprintf(stderr, "Block size must be pow of 2!\n");
                return NULL;
        }

        if (block_size < (int) sizeof(node)) {
                fprintf(stderr, "block size is too small for one node!\n");
                return NULL;
        }

        _block_size = block_size;
        _max_order = (block_size - sizeof(node)) / (sizeof(key_t) + sizeof(off_t));
        _max_entries = (block_size - sizeof(node)) / (sizeof(key_t) + sizeof(long));
        if (_max_order <= 2) {
                fprintf(stderr, "block size is too small for one node!\n");
                return NULL;
        }

        struct bplus_tree *tree = calloc(1, sizeof(*tree));
        assert(tree != NULL);
        list_init(&tree->free_blocks);
        strcpy(tree->filename, filename);

        /* load index boot file */
        int fd = open(strcat(tree->filename, ".boot"), O_RDWR, 0644);
        if (fd >= 0) {
                tree->root = offset_load(fd);
                _block_size = offset_load(fd);
                tree->file_size = offset_load(fd);
                /* load free blocks */
                while ((i = offset_load(fd)) != INVALID_OFFSET) {
                        struct free_block *block = malloc(sizeof(*block));
                        assert(block != NULL);
                        block->offset = i;
                        list_add(&block->link, &tree->free_blocks);
                }
                close(fd);
        } else {
                tree->root = INVALID_OFFSET;
                _block_size = block_size;
                tree->file_size = 0;
        }

        /* set order and entries */
        _max_order = (_block_size - sizeof(node)) / (sizeof(key_t) + sizeof(off_t));
        _max_entries = (_block_size - sizeof(node)) / (sizeof(key_t) + sizeof(long));
        printf("config node order:%d and leaf entries:%d\n", _max_order, _max_entries);

        /* init free node caches */
        tree->caches = malloc(_block_size * MIN_CACHE_NUM);

        /* open data file */
        tree->fd = bplus_open(filename);
        assert(tree->fd >= 0);
        return tree;
}

void bplus_tree_deinit(struct bplus_tree *tree)
{
        int fd = open(tree->filename, O_CREAT | O_RDWR, 0644);
        assert(fd >= 0);
        assert(offset_store(fd, tree->root) == ADDR_STR_WIDTH);
        assert(offset_store(fd, _block_size) == ADDR_STR_WIDTH);
        assert(offset_store(fd, tree->file_size) == ADDR_STR_WIDTH);

        /* store free blocks in files for future reuse */
        struct list_head *pos, *n;
        list_for_each_safe(pos, n, &tree->free_blocks) {
                list_del(pos);
                struct free_block *block = list_entry(pos, struct free_block, link);
                assert(offset_store(fd, block->offset) == ADDR_STR_WIDTH);
                free(block);
        }

        bplus_close(tree->fd);
        free(tree->caches);
        free(tree);
}

#ifdef _BPLUS_TREE_DEBUG

#define MAX_LEVEL 10

struct node_backlog {
        /* Node backlogged */
        off_t offset;
        /* The index next to the backtrack point, must be >= 1 */
        int next_sub_idx;
};

static inline int children(struct bplus_node *node)
{
        assert(!is_leaf(node));
        return node->children;
}

static void node_key_dump(struct bplus_node *node)
{
        int i;
        if (is_leaf(node)) {
                printf("leaf:");
                for (i = 0; i < node->children; i++) {
                        printf(" %d", key(node)[i]);
                }
        } else {
                printf("node:");
                for (i = 0; i < node->children - 1; i++) {
                        printf(" %d", key(node)[i]);
                }
        }
        printf("\n");
}

static void draw(struct bplus_tree *tree, struct bplus_node *node, struct node_backlog *stack, int level)
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

void bplus_tree_dump(struct bplus_tree *tree)
{
        int level = 0;
        struct bplus_node *node = node_seek(tree, tree->root);
        struct node_backlog *p_nbl = NULL;
        struct node_backlog nbl_stack[MAX_LEVEL];
        struct node_backlog *top = nbl_stack;

        for (; ;) {
                if (node != NULL) {
                        /* non-zero needs backward and zero does not */
                        int sub_idx = p_nbl != NULL ? p_nbl->next_sub_idx : 0;
                        /* Reset each loop */
                        p_nbl = NULL;

                        /* Backlog the node */
                        if (is_leaf(node) || sub_idx + 1 >= children(node)) {
                                top->offset = INVALID_OFFSET;
                                top->next_sub_idx = 0;
                        } else {
                                top->offset = node->self;
                                top->next_sub_idx = sub_idx + 1;
                        }
                        top++;
                        level++;

                        /* Draw the node when first passed through */
                        if (sub_idx == 0) {
                                draw(tree, node, nbl_stack, level);
                        }

                        /* Move deep down */
                        node = is_leaf(node) ? NULL : node_seek(tree, sub(node)[sub_idx]);
                } else {
                        p_nbl = top == nbl_stack ? NULL : --top;
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
