/*
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "bplustree.h"

enum {
        BPLUS_TREE_LEAF,
        BPLUS_TREE_NON_LEAF = 1,
};

enum {
        LEFT_SIBLING,
        RIGHT_SIBLING = 1,
};

static int
key_binary_search(int *arr, int len, int target)
{
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

static struct bplus_non_leaf *
non_leaf_new(void)
{
        struct bplus_non_leaf *node = calloc(1, sizeof(*node));
        assert(node != NULL);
        list_init(&node->link);
        node->type = BPLUS_TREE_NON_LEAF;
        node->parent_key_idx = -1;
        return node;
}

static struct bplus_leaf *
leaf_new(void)
{
        struct bplus_leaf *node = calloc(1, sizeof(*node));
        assert(node != NULL);
        list_init(&node->link);
        node->type = BPLUS_TREE_LEAF;
        node->parent_key_idx = -1;
        return node;
}

static void
non_leaf_delete(struct bplus_non_leaf *node)
{
        list_del(&node->link);
        free(node);
}

static void
leaf_delete(struct bplus_leaf *node)
{
        list_del(&node->link);
        free(node);
}

static int
bplus_tree_search(struct bplus_tree *tree, int key)
{
        int i;
        struct bplus_node *node = tree->root;
        struct bplus_non_leaf *nln;
        struct bplus_leaf *ln;

        while (node != NULL) {
                switch (node->type) {
                case BPLUS_TREE_NON_LEAF:
                        nln = (struct bplus_non_leaf *)node;
                        i = key_binary_search(nln->key, nln->children - 1, key);
                        if (i >= 0) {
                                node = nln->sub_ptr[i + 1];
                        } else {
                                i = -i - 1;
                                node = nln->sub_ptr[i];
                        }
                        break;
                case BPLUS_TREE_LEAF:
                        ln = (struct bplus_leaf *)node;
                        i = key_binary_search(ln->key, ln->entries, key);
                        if (i >= 0) {
                                return ln->data[i];
                        } else {
                                return 0;
                        }
                default:
                        assert(0);
                }
        }

        return 0;
}

static int
non_leaf_insert(struct bplus_tree *tree, struct bplus_non_leaf *node, struct bplus_node *sub_node, int key)
{
        int i, j, split_key;
        int split = 0;
        struct bplus_non_leaf *sibling;

        int insert = key_binary_search(node->key, node->children - 1, key);
        assert(insert < 0);
        insert = -insert - 1;

        /* node full */
        if (node->children == tree->order) {
                /* split = [m/2] */
                split = (tree->order + 1) / 2;
                /* splited sibling node */
                sibling = non_leaf_new();
                list_add(&sibling->link, &node->link);
                /* non-leaf node's children always equals to split + 1 after insertion */
                node->children = split + 1;
                /* sibling node replication due to location of insertion */
                if (insert < split) {
                        split_key = node->key[split - 1];
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = node->sub_ptr[split];
                        sibling->sub_ptr[0]->parent = sibling;
                        sibling->sub_ptr[0]->parent_key_idx = -1;
                        /* insertion point is before split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->order - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                sibling->sub_ptr[j + 1]->parent = sibling;
                                sibling->sub_ptr[j + 1]->parent_key_idx = j;
                        }
                        sibling->children = j + 1;
                        /* insert new key and sub-node */
                        for (i = node->children - 2; i > insert; i--) {
                                node->key[i] = node->key[i - 1];
                                node->sub_ptr[i + 1] = node->sub_ptr[i];
                                node->sub_ptr[i + 1]->parent_key_idx = i;
                        }
                        node->key[i] = key;
                        node->sub_ptr[i + 1] = sub_node;
                        node->sub_ptr[i + 1]->parent = node;
                        node->sub_ptr[i + 1]->parent_key_idx = i;
                } else if (insert == split) {
                        split_key = key;
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = sub_node;
                        sibling->sub_ptr[0]->parent = sibling;
                        sibling->sub_ptr[0]->parent_key_idx = -1;
                        /* insertion point is split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->order - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                sibling->sub_ptr[j + 1]->parent = sibling;
                                sibling->sub_ptr[j + 1]->parent_key_idx = j;
                        }
                        sibling->children = j + 1;
                } else {
                        split_key = node->key[split];
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = node->sub_ptr[split + 1];
                        sibling->sub_ptr[0]->parent = sibling;
                        sibling->sub_ptr[0]->parent_key_idx = -1;
                        /* insertion point is after split point, replicate from key[split + 1] */
                        for (i = split + 1, j = 0; i < tree->order - 1; j++) {
                                if (j != insert - split - 1) {
                                        sibling->key[j] = node->key[i];
                                        sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                        sibling->sub_ptr[j + 1]->parent = sibling;
                                        sibling->sub_ptr[j + 1]->parent_key_idx = j;
                                        i++;
                                }
                        }
                        /* reserve a hole for insertion */
                        if (j > insert - split - 1) {
                                sibling->children = j + 1;
                        } else {
                                assert(j == insert - split - 1);
                                sibling->children = j + 2;
                        }
                        /* insert new key and sub-node*/
                        j = insert - split - 1;
                        sibling->key[j] = key;
                        sibling->sub_ptr[j + 1] = sub_node;
                        sibling->sub_ptr[j + 1]->parent = sibling;
                        sibling->sub_ptr[j + 1]->parent_key_idx = j;
                }
        } else {
                /* simple insertion */
                for (i = node->children - 1; i > insert; i--) {
                        node->key[i] = node->key[i - 1];
                        node->sub_ptr[i + 1] = node->sub_ptr[i];
                        node->sub_ptr[i + 1]->parent_key_idx = i;
                }
                node->key[i] = key;
                node->sub_ptr[i + 1] = sub_node;
                node->sub_ptr[i + 1]->parent_key_idx = i;
                node->children++;
        }

        if (split) {
                struct bplus_non_leaf *parent = node->parent;
                if (parent == NULL) {
                        /* new parent */
                        parent = non_leaf_new();
                        parent->key[0] = split_key;
                        parent->sub_ptr[0] = (struct bplus_node *)node;
                        parent->sub_ptr[0]->parent = parent;
                        parent->sub_ptr[0]->parent_key_idx = -1;
                        parent->sub_ptr[1] = (struct bplus_node *)sibling;
                        parent->sub_ptr[1]->parent = parent;
                        parent->sub_ptr[1]->parent_key_idx = 0;
                        parent->children = 2;
                        /* update root */
                        tree->root = (struct bplus_node *)parent;
                        list_add(&parent->link, &tree->list[++tree->level]);
                } else {
                        /* Trace upwards */
                        sibling->parent = parent;
                        return non_leaf_insert(tree, parent, (struct bplus_node *)sibling, split_key);
                }
        }

        return 0;
}

static int
leaf_insert(struct bplus_tree *tree, struct bplus_leaf *leaf, int key, int data)
{
        int i, j, split = 0;
        struct bplus_leaf *sibling;

        int insert = key_binary_search(leaf->key, leaf->entries, key);
        if (insert >= 0) {
                /* Already exists */
                return -1;
        }
        insert = -insert - 1;

        /* node full */
        if (leaf->entries == tree->entries) {
                /* split = [m/2] */
                split = (tree->entries + 1) / 2;
                /* splited sibling node */
                sibling = leaf_new();
                list_add(&sibling->link, &leaf->link);
                /* leaf node's entries always equals to split after insertion */
                leaf->entries = split;
                /* sibling leaf replication due to location of insertion */
                if (insert < split) {
                        /* insertion point is before split point, replicate from key[split - 1] */
                        for (i = split - 1, j = 0; i < tree->entries; i++, j++) {
                                sibling->key[j] = leaf->key[i];
                                sibling->data[j] = leaf->data[i];
                        }
                        sibling->entries = j;
                        /* insert new key and sub-node */
                        for (i = split - 1; i > insert; i--) {
                                leaf->key[i] = leaf->key[i - 1];
                                leaf->data[i] = leaf->data[i - 1];
                        }
                        leaf->key[i] = key;
                        leaf->data[i] = data;
                } else {
                        /* insertion point is or after split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->entries; j++) {
                                if (j != insert - split) {
                                        sibling->key[j] = leaf->key[i];
                                        sibling->data[j] = leaf->data[i];
                                        i++;
                                }
                        }
                        /* reserve a hole for insertion */
                        if (j > insert - split) {
                                sibling->entries = j;
                        } else {
                                assert(j == insert - split);
                                sibling->entries = j + 1;
                        }
                        /* insert new key */
                        j = insert - split;
                        sibling->key[j] = key;
                        sibling->data[j] = data;
                }
        } else {
                /* simple insertion */
                for (i = leaf->entries; i > insert; i--) {
                        leaf->key[i] = leaf->key[i - 1];
                        leaf->data[i] = leaf->data[i - 1];
                }
                leaf->key[i] = key;
                leaf->data[i] = data;
                leaf->entries++;
        }

        if (split) {
                struct bplus_non_leaf *parent = leaf->parent;
                if (parent == NULL) {
                        /* new parent */
                        parent = non_leaf_new();
                        parent->key[0] = sibling->key[0];
                        parent->sub_ptr[0] = (struct bplus_node *)leaf;
                        parent->sub_ptr[0]->parent = parent;
                        parent->sub_ptr[0]->parent_key_idx = -1;
                        parent->sub_ptr[1] = (struct bplus_node *)sibling;
                        parent->sub_ptr[1]->parent = parent;
                        parent->sub_ptr[1]->parent_key_idx = 0;
                        parent->children = 2;
                        /* update root */
                        tree->root = (struct bplus_node *)parent;
                        list_add(&parent->link, &tree->list[++tree->level]);
                } else {
                        sibling->parent = parent;
                        return non_leaf_insert(tree, parent, (struct bplus_node *) sibling, sibling->key[0]);
                }
        }

        return 0;
}

static int
bplus_tree_insert(struct bplus_tree *tree, int key, int data)
{
        int i;
        struct bplus_node *node = tree->root;
        struct bplus_non_leaf *nln;
        struct bplus_leaf *ln, *root;

        while (node != NULL) {
                switch (node->type) {
                case BPLUS_TREE_NON_LEAF:
                        nln = (struct bplus_non_leaf *)node;
                        i = key_binary_search(nln->key, nln->children - 1, key);
                        if (i >= 0) {
                                node = nln->sub_ptr[i + 1];
                        } else {
                                i = -i - 1;
                                node = nln->sub_ptr[i];
                        }
                        break;
                case BPLUS_TREE_LEAF:
                        ln = (struct bplus_leaf *)node;
                        return leaf_insert(tree, ln, key, data);
                default:
                        assert(0);
                }
        }

        /* new root */
        root = leaf_new();
        root->key[0] = key;
        root->data[0] = data;
        root->entries = 1;
        tree->root = (struct bplus_node *)root;
        list_add(&root->link, &tree->list[tree->level]);
        return 0;
}

static void
non_leaf_shift_from_left(struct bplus_non_leaf *node, int parent_key_index, int remove)
{
        int i;
        struct bplus_non_leaf *sibling = list_prev_entry(node, link);
        /* node's elements right shift */
        for (i = remove; i > 0; i--) {
                node->key[i] = node->key[i - 1];
        }
        for (i = remove + 1; i > 0; i--) {
                node->sub_ptr[i] = node->sub_ptr[i - 1];
                node->sub_ptr[i]->parent_key_idx = i - 1;
        }
        /* parent key right rotation */
        node->key[0] = node->parent->key[parent_key_index];
        node->parent->key[parent_key_index] = sibling->key[sibling->children - 2];
        /* borrow the last sub-node from left sibling */
        node->sub_ptr[0] = sibling->sub_ptr[sibling->children - 1];
        node->sub_ptr[0]->parent = node;
        node->sub_ptr[0]->parent_key_idx = -1;
        sibling->children--;
}

static void
non_leaf_merge_into_left(struct bplus_non_leaf *node, int parent_key_index, int remove)
{
        int i, j;
        struct bplus_non_leaf *sibling = list_prev_entry(node, link);
        /* move parent key down */
        sibling->key[sibling->children - 1] = node->parent->key[parent_key_index];
        /* merge into left sibling */
        for (i = sibling->children, j = 0; j < node->children - 1; j++) {
                if (j != remove) {
                        sibling->key[i] = node->key[j];
                        i++;
                }
        }
        for (i = sibling->children, j = 0; j < node->children; j++) {
                if (j != remove + 1) {
                        sibling->sub_ptr[i] = node->sub_ptr[j];
                        sibling->sub_ptr[i]->parent = sibling;
                        sibling->sub_ptr[i]->parent_key_idx = i - 1;
                        i++;
                }
        }
        sibling->children = i;
        /* delete empty node */
        non_leaf_delete(node);
}

static void
non_leaf_shift_from_right(struct bplus_non_leaf *node, int parent_key_index)
{
        int i;
        struct bplus_non_leaf *sibling = list_next_entry(node, link);
        /* parent key left rotation */
        node->key[node->children - 1] = node->parent->key[parent_key_index];
        node->parent->key[parent_key_index] = sibling->key[0];
        /* borrow the frist sub-node from right sibling */
        node->sub_ptr[node->children] = sibling->sub_ptr[0];
        node->sub_ptr[node->children]->parent = node;
        node->sub_ptr[node->children]->parent_key_idx = node->children - 1;
        node->children++;
        /* left shift in right sibling */
        for (i = 0; i < sibling->children - 2; i++) {
                sibling->key[i] = sibling->key[i + 1];
        }
        for (i = 0; i < sibling->children - 1; i++) {
                sibling->sub_ptr[i] = sibling->sub_ptr[i + 1];
                sibling->sub_ptr[i]->parent_key_idx = i - 1;
        }
        sibling->children--;
}

static void
non_leaf_merge_from_right(struct bplus_non_leaf *node, int parent_key_index)
{
        int i, j;
        struct bplus_non_leaf *sibling = list_next_entry(node, link);
        /* move parent key down */
        node->key[node->children - 1] = node->parent->key[parent_key_index];
        node->children++;
        /* merge from right sibling */
        for (i = node->children - 1, j = 0; j < sibling->children - 1; i++, j++) {
                node->key[i] = sibling->key[j];
        }
        for (i = node->children - 1, j = 0; j < sibling->children; i++, j++) {
                node->sub_ptr[i] = sibling->sub_ptr[j];
                node->sub_ptr[i]->parent = node;
                node->sub_ptr[i]->parent_key_idx = i - 1;
        }
        node->children = i;
        /* delete empty sibling */
        non_leaf_delete(sibling);
}

static void
non_leaf_remove(struct bplus_tree *tree, struct bplus_non_leaf *node, int remove)
{
        int i;
        struct bplus_non_leaf *sibling;

        if (node->children <= (tree->order + 1) / 2) {
                struct bplus_non_leaf *parent = node->parent;
                if (parent != NULL) {
                        int borrow = 0;
                        /* decide which sibling to be borrowed from */
                        i = node->parent_key_idx;
                        if (i == -1) {
                                /* the frist sub-node, no left sibling, choose the right one */
                                sibling = list_next_entry(node, link);
                                borrow = RIGHT_SIBLING;
                        } else if (i == parent->children - 2) {
                                /* the last sub-node, no right sibling, choose the left one */
                                sibling = list_prev_entry(node, link);
                                borrow = LEFT_SIBLING;
                        } else {
                                /* if both left and right sibling found, choose the one with more children */
                                struct bplus_non_leaf *l_sib = list_prev_entry(node, link);
                                struct bplus_non_leaf *r_sib = list_next_entry(node, link);
                                sibling = l_sib->children >= r_sib->children ? l_sib : r_sib;
                                borrow = l_sib->children >= r_sib->children ? LEFT_SIBLING : RIGHT_SIBLING;
                        }

                        if (borrow == LEFT_SIBLING) {
                                if (sibling->children > (tree->order + 1) / 2) {
                                        non_leaf_shift_from_left(node, i, remove);
                                } else {
                                        non_leaf_merge_into_left(node, i, remove);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i);
                                }
                        } else {
                                /* remove key first in case of overflow during merging with sibling node */
                                for (; remove < node->children - 2; remove++) {
                                        node->key[remove] = node->key[remove + 1];
                                        node->sub_ptr[remove + 1] = node->sub_ptr[remove + 2];
                                        node->sub_ptr[remove + 1]->parent_key_idx = remove;
                                }
                                node->children--;
                                if (sibling->children > (tree->order + 1) / 2) {
                                        non_leaf_shift_from_right(node, i + 1);
                                } else {
                                        non_leaf_merge_from_right(node, i + 1);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1);
                                }
                        }
                        /* deletion finishes */
                        return;
                } else {
                        if (node->children == 2) {
                                /* delete old root node */
                                assert(remove == 0);
                                node->sub_ptr[0]->parent = NULL;
                                tree->root = node->sub_ptr[0];
                                non_leaf_delete(node);
                                return;
                        }
                }
        }
        
        /* simple deletion */
        assert(node->children > 2);
        for (; remove < node->children - 2; remove++) {
                node->key[remove] = node->key[remove + 1];
                node->sub_ptr[remove + 1] = node->sub_ptr[remove + 2];
                node->sub_ptr[remove + 1]->parent_key_idx = remove;
        }
        node->children--;
}

static void
leaf_shift_from_left(struct bplus_leaf *leaf, int parent_key_index, int remove)
{
        struct bplus_leaf *sibling = list_prev_entry(leaf, link);
        /* right shift in leaf node */
        for (; remove > 0; remove--) {
                leaf->key[remove] = leaf->key[remove - 1];
                leaf->data[remove] = leaf->data[remove - 1];
        }
        /* borrow the last element from left sibling */
        leaf->key[0] = sibling->key[sibling->entries - 1];
        leaf->data[0] = sibling->data[sibling->entries - 1];
        sibling->entries--;
        /* update parent key */
        leaf->parent->key[parent_key_index] = leaf->key[0];
}

static void
leaf_merge_into_left(struct bplus_leaf *leaf, int remove)
{
        int i, j;
        struct bplus_leaf *sibling = list_prev_entry(leaf, link);
        /* merge into left sibling */
        for (i = sibling->entries, j = 0; j < leaf->entries; j++) {
                if (j != remove) {
                        sibling->key[i] = leaf->key[j];
                        sibling->data[i] = leaf->data[j];
                        i++;
                }
        }
        sibling->entries = i;
        /* delete merged leaf */
        leaf_delete(leaf);
}

static void
leaf_shift_from_right(struct bplus_leaf *leaf, int parent_key_index)
{
        int i;
        struct bplus_leaf *sibling = list_next_entry(leaf, link);
        /* borrow the first element from right sibling */
        leaf->key[leaf->entries] = sibling->key[0];
        leaf->data[leaf->entries] = sibling->data[0];
        leaf->entries++;
        /* left shift in right sibling */
        for (i = 0; i < sibling->entries - 1; i++) {
                sibling->key[i] = sibling->key[i + 1];
                sibling->data[i] = sibling->data[i + 1];
        }
        sibling->entries--;
        /* update parent key */
        leaf->parent->key[parent_key_index] = sibling->key[0];
}

static void
leaf_merge_from_right(struct bplus_leaf *leaf)
{
        int i, j;
        struct bplus_leaf *sibling = list_next_entry(leaf, link);
        /* merge from right sibling */
        for (i = leaf->entries, j = 0; j < sibling->entries; i++, j++) {
                leaf->key[i] = sibling->key[j];
                leaf->data[i] = sibling->data[j];
        }
        leaf->entries = i;
        /* delete right sibling */
        leaf_delete(sibling);
}

static int
leaf_remove(struct bplus_tree *tree, struct bplus_leaf *leaf, int key)
{
        int i, j, k;
        struct bplus_leaf *sibling;

        int remove = key_binary_search(leaf->key, leaf->entries, key);
        if (remove < 0) {
                /* Not exist */
                return -1;
        }

        if (leaf->entries <= (tree->entries + 1) / 2) {
                struct bplus_non_leaf *parent = leaf->parent;
                if (parent != NULL) {
                        int borrow = 0;
                        /* decide which sibling to be borrowed from */
                        i = leaf->parent_key_idx;
                        if (i == -1) {
                                /* the frist sub-node, no left sibling, choose the right one */
                                sibling = list_next_entry(leaf, link);
                                borrow = RIGHT_SIBLING;
                        } else if (i == parent->children - 2) {
                                /* the last sub-node, no right sibling, choose the left one */
                                sibling = list_prev_entry(leaf, link);
                                borrow = LEFT_SIBLING;
                        } else {
                                /* if both left and right sibling found, choose the one with more entries */
                                struct bplus_leaf *l_sib = list_prev_entry(leaf, link);
                                struct bplus_leaf *r_sib = list_next_entry(leaf, link);
                                sibling = l_sib->entries >= r_sib->entries ? l_sib : r_sib;
                                borrow = l_sib->entries >= r_sib->entries ? LEFT_SIBLING : RIGHT_SIBLING;
                        }

                        if (borrow == LEFT_SIBLING) {
                                if (sibling->entries > (tree->entries + 1) / 2) {
                                        leaf_shift_from_left(leaf, i, remove);
                                } else {
                                        leaf_merge_into_left(leaf, remove);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i);
                                }
                        } else {
                                /* remove element first in case of overflow during merging with sibling node */
                                for (; remove < leaf->entries - 1; remove++) {
                                        leaf->key[remove] = leaf->key[remove + 1];
                                        leaf->data[remove] = leaf->data[remove + 1];
                                }
                                leaf->entries--;
                                if (sibling->entries > (tree->entries + 1) / 2) {
                                        leaf_shift_from_right(leaf, i + 1);
                                } else {
                                        leaf_merge_from_right(leaf);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1);
                                }
                        }
                        /* deletion finishes */
                        return 0;
                } else {
                        if (leaf->entries == 1) {
                                /* delete the onbly last node */
                                assert(key == leaf->key[0]);
                                tree->root = NULL;
                                leaf_delete(leaf);
                                return 0;
                        }
                }
        }

        /* simple deletion */
        for (; remove < leaf->entries - 1; remove++) {
                leaf->key[remove] = leaf->key[remove + 1];
                leaf->data[remove] = leaf->data[remove + 1];
        }
        leaf->entries--;

        return 0;
}

static int
bplus_tree_delete(struct bplus_tree *tree, int key)
{
        int i;
        struct bplus_node *node = tree->root;
        struct bplus_non_leaf *nln;
        struct bplus_leaf *ln;

        while (node != NULL) {
                switch (node->type) {
                case BPLUS_TREE_NON_LEAF:
                        nln = (struct bplus_non_leaf *)node;
                        i = key_binary_search(nln->key, nln->children - 1, key);
                        if (i >= 0) {
                                node = nln->sub_ptr[i + 1];
                        } else {
                                i = -i - 1;
                                node = nln->sub_ptr[i];
                        }
                        break;
                case BPLUS_TREE_LEAF:
                        ln = (struct bplus_leaf *)node;
                        return leaf_remove(tree, ln, key);
                default:
                        assert(0);
                }
        }

        return -1;
}

int
bplus_tree_get(struct bplus_tree *tree, int key)
{
        int data = bplus_tree_search(tree, key); 
        if (data) {
                return data;
        } else {
                return -1;
        }
}

int
bplus_tree_put(struct bplus_tree *tree, int key, int data)
{
        if (data) {
                return bplus_tree_insert(tree, key, data);
        } else {
                return bplus_tree_delete(tree, key);
        }
}

struct bplus_tree *
bplus_tree_init(int order, int entries)
{
        int i;
        /* The max order of non leaf nodes must be more than two */
        assert(BPLUS_MAX_ORDER > BPLUS_MIN_ORDER);
        assert(order <= BPLUS_MAX_ORDER && entries <= BPLUS_MAX_ENTRIES);

        struct bplus_tree *tree = calloc(1, sizeof(*tree));
        if (tree != NULL) {
                tree->root = NULL;
                tree->order = order;
                tree->entries = entries;
                for (i = 0; i < BPLUS_MAX_LEVEL; i++) {
                        list_init(&tree->list[i]);
                }
        }

        return tree;
}

void
bplus_tree_deinit(struct bplus_tree *tree)
{
        free(tree);
}

int
bplus_tree_get_range(struct bplus_tree *tree, int key1, int key2)
{
    int i, data = 0;
    int min = key1 <= key2 ? key1 : key2;
    int max = min == key1 ? key2 : key1;
    struct bplus_node *node = tree->root;
    struct bplus_non_leaf *nln;
    struct bplus_leaf *ln;

    while (node != NULL) {
            switch (node->type) {
            case BPLUS_TREE_NON_LEAF:
                    nln = (struct bplus_non_leaf *)node;
                    i = key_binary_search(nln->key, nln->children - 1, min);
                    if (i >= 0) {
                            node = nln->sub_ptr[i + 1];
                    } else  {
                            i = -i - 1;
                            node = nln->sub_ptr[i];
                    }
                    break;
            case BPLUS_TREE_LEAF:
                    ln = (struct bplus_leaf *)node;
                    i = key_binary_search(ln->key, ln->entries, min);
                    if (i < 0) {
                            i = -i - 1;
                            if (i >= ln->entries) {
                                    ln = list_next_entry(ln, link);
                            }
                    }
                    while (ln != NULL && ln->key[i] <= max) {
                            data = ln->data[i];
                            if (++i >= ln->entries) {
                                    ln = list_next_entry(ln, link);
                                    i = 0;
                            }
                    }
                    return data;
            default:
                    assert(0);
            }
    }

    return 0;
}

#ifdef _BPLUS_TREE_DEBUG
struct node_backlog {
        /* Node backlogged */
        struct bplus_node *node;
        /* The index next to the backtrack point, must be >= 1 */
        int next_sub_idx;
};

static inline void
nbl_push(struct node_backlog *nbl, struct node_backlog **top, struct node_backlog **buttom)
{
        if (*top - *buttom < BPLUS_MAX_LEVEL) {
                (*(*top)++) = *nbl;
        }
}

static inline struct node_backlog *
nbl_pop(struct node_backlog **top, struct node_backlog **buttom)
{
        return *top - *buttom > 0 ? --*top : NULL;
}

static inline int
is_leaf(struct bplus_node *node)
{
        return node->type == BPLUS_TREE_LEAF;
}

static inline int
children(struct bplus_node *node)
{
        return ((struct bplus_non_leaf *) node)->children;
}

static void
key_print(struct bplus_node *node)
{
        int i;
        if (is_leaf(node)) {
                struct bplus_leaf *leaf = (struct bplus_leaf *) node;
                printf("leaf:");
                for (i = 0; i < leaf->entries; i++) {
                        printf(" %d", leaf->key[i]);
                }
        } else {
                struct bplus_non_leaf *non_leaf = (struct bplus_non_leaf *) node;
                printf("node:");
                for (i = 0; i < non_leaf->children - 1; i++) {
                        printf(" %d", non_leaf->key[i]);
                }
        }
        printf("\n");
}

void
bplus_tree_dump(struct bplus_tree *tree)
{
        int level = 0;
        struct bplus_node *node = tree->root;
        struct node_backlog nbl, *p_nbl = NULL;
        struct node_backlog *top, *buttom, nbl_stack[BPLUS_MAX_LEVEL];

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
                                nbl.node = NULL;
                                nbl.next_sub_idx = 0;
                        } else {
                                nbl.node = node;
                                nbl.next_sub_idx = sub_idx + 1;
                        }
                        nbl_push(&nbl, &top, &buttom);
                        level++;

                        /* Draw lines as long as sub_idx is the first one */
                        if (sub_idx == 0) {
                                int i;
                                for (i = 1; i < level; i++) {
                                        if (i == level - 1) {
                                                printf("%-8s", "+-------");
                                        } else {
                                                if (nbl_stack[i - 1].node != NULL) {
                                                        printf("%-8s", "|");
                                                } else {
                                                        printf("%-8s", " ");
                                                }
                                        }
                                }
                                key_print(node);
                        }

                        /* Move deep down */
                        node = is_leaf(node) ? NULL : ((struct bplus_non_leaf *) node)->sub_ptr[sub_idx];
                } else {
                        p_nbl = nbl_pop(&top, &buttom);
                        if (p_nbl == NULL) {
                                /* End of traversal */
                                break;
                        }
                        node = p_nbl->node;
                        level--;
                }
        }
}
#endif
