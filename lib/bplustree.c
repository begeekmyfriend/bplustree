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
        BORROW_FROM_LEFT,
        BORROW_FROM_RIGHT = 1,
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
        node->type = BPLUS_TREE_NON_LEAF;
        return node;
}

static struct bplus_leaf *
leaf_new(void)
{
        struct bplus_leaf *node = calloc(1, sizeof(*node));
        assert(node != NULL);
        node->type = BPLUS_TREE_LEAF;
        return node;
}

static void
non_leaf_delete(struct bplus_non_leaf *node)
{
        free(node);
}

static void
leaf_delete(struct bplus_leaf *node)
{
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
non_leaf_insert(struct bplus_tree *tree, struct bplus_non_leaf *node, struct bplus_node *sub_node, int key, int level)
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
                sibling->next = node->next;
                node->next = sibling;
                /* non-leaf node's children always equals to split + 1 after insertion */
                node->children = split + 1;
                /* sibling node replication due to location of insertion */
                if (insert < split) {
                        split_key = node->key[split - 1];
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = node->sub_ptr[split];
                        node->sub_ptr[split]->parent = sibling;
                        /* insertion point is before split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->order - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                node->sub_ptr[i + 1]->parent = sibling;
                        }
                        sibling->children = j + 1;
                        /* insert new key and sub-node */
                        for (i = node->children - 2; i > insert; i--) {
                                node->key[i] = node->key[i - 1];
                                node->sub_ptr[i + 1] = node->sub_ptr[i];
                        }
                        node->key[i] = key;
                        node->sub_ptr[i + 1] = sub_node;
                        sub_node->parent = node;
                } else if (insert == split) {
                        split_key = key;
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = sub_node;
                        sub_node->parent = sibling;
                        /* insertion point is split point, replicate from key[split] */
                        for (i = split, j = 0; i < tree->order - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                node->sub_ptr[i + 1]->parent = sibling;
                        }
                        sibling->children = j + 1;
                } else {
                        split_key = node->key[split];
                        /* sibling node's first sub-node */
                        sibling->sub_ptr[0] = node->sub_ptr[split + 1];
                        node->sub_ptr[split + 1]->parent = sibling;
                        /* insertion point is after split point, replicate from key[split + 1] */
                        for (i = split + 1, j = 0; i < tree->order - 1; j++) {
                                if (j != insert - split - 1) {
                                        sibling->key[j] = node->key[i];
                                        sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                        node->sub_ptr[i + 1]->parent = sibling;
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
                        sub_node->parent = sibling;
                }
        } else {
                /* simple insertion */
                for (i = node->children - 1; i > insert; i--) {
                        node->key[i] = node->key[i - 1];
                        node->sub_ptr[i + 1] = node->sub_ptr[i];
                }
                node->key[i] = key;
                node->sub_ptr[i + 1] = sub_node;
                node->children++;
        }

        if (split) {
                struct bplus_non_leaf *parent = node->parent;
                if (parent == NULL) {
                        if (++level >= tree->level) {
                                fprintf(stderr, "!!Panic: Level exceeded, please expand the tree level, non-leaf order or leaf entries for element capacity!\n");
                                node->next = sibling->next;
                                non_leaf_delete(sibling);
                                return -1;
                        }
                        /* new parent */
                        parent = non_leaf_new();
                        parent->key[0] = split_key;
                        parent->sub_ptr[0] = (struct bplus_node *)node;
                        parent->sub_ptr[1] = (struct bplus_node *)sibling;
                        parent->children = 2;
                        /* update root */
                        tree->root = (struct bplus_node *)parent;
                        tree->head[level] = (struct bplus_node *)parent;
                        node->parent = parent;
                        sibling->parent = parent;
                } else {
                        /* Trace upwards */
                        sibling->parent = parent;
                        return non_leaf_insert(tree, parent, (struct bplus_node *)sibling, split_key, level + 1);
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
                sibling->next = leaf->next;
                leaf->next = sibling;
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
                        parent->sub_ptr[1] = (struct bplus_node *)sibling;
                        parent->children = 2;
                        /* update root */
                        tree->root = (struct bplus_node *)parent;
                        tree->head[1] = (struct bplus_node *)parent;
                        leaf->parent = parent;
                        sibling->parent = parent;
                } else {
                        /* trace upwards */
                        sibling->parent = parent;
                        return non_leaf_insert(tree, parent, (struct bplus_node *)sibling, sibling->key[0], 1);
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
        tree->head[0] = (struct bplus_node *)root;
        tree->root = (struct bplus_node *)root;
        return 0;
}

static void
non_leaf_remove(struct bplus_tree *tree, struct bplus_non_leaf *node, int remove, int level)
{
        int i, j, k;
        struct bplus_non_leaf *sibling;

        if (node->children <= (tree->order + 1) / 2) {
                struct bplus_non_leaf *parent = node->parent;
                if (parent != NULL) {
                        int borrow = 0;
                        /* find which sibling node with same parent to be borrowed from */
                        i = key_binary_search(parent->key, parent->children - 1, node->key[0]);
                        assert(i < 0);
                        i = -i - 1;
                        if (i == 0) {
                                /* no left sibling, choose right one */
                                sibling = (struct bplus_non_leaf *)parent->sub_ptr[i + 1];
                                borrow = BORROW_FROM_RIGHT;
                        } else if (i == parent->children - 1) {
                                /* no right sibling, choose left one */
                                sibling = (struct bplus_non_leaf *)parent->sub_ptr[i - 1];
                                borrow = BORROW_FROM_LEFT;
                        } else {
                                struct bplus_non_leaf *l_sib = (struct bplus_non_leaf *)parent->sub_ptr[i - 1];
                                struct bplus_non_leaf *r_sib = (struct bplus_non_leaf *)parent->sub_ptr[i + 1];
                                /* if both left and right sibling found, choose the one with more children */
                                sibling = l_sib->children >= r_sib->children ? l_sib : r_sib;
                                borrow = l_sib->children >= r_sib->children ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                        }

                        /* locate parent node key to update later */
                        i = i - 1;

                        if (borrow == BORROW_FROM_LEFT) {
                                if (sibling->children > (tree->order + 1) / 2) {
                                        /* node's elements right shift */
                                        for (j = remove; j > 0; j--) {
                                                node->key[j] = node->key[j - 1];
                                        }
                                        for (j = remove + 1; j > 0; j--) {
                                                node->sub_ptr[j] = node->sub_ptr[j - 1];
                                        }
                                        /* parent key right rotation */
                                        node->key[0] = parent->key[i];
                                        parent->key[i] = sibling->key[sibling->children - 2];
                                        /* borrow the last sub-node from left sibling */
                                        node->sub_ptr[0] = sibling->sub_ptr[sibling->children - 1];
                                        sibling->sub_ptr[sibling->children - 1]->parent = node;
                                        sibling->children--;
                                } else {
                                        /* move parent key down */
                                        sibling->key[sibling->children - 1] = parent->key[i];
                                        /* merge with left sibling */
                                        for (j = sibling->children, k = 0; k < node->children - 1; k++) {
                                                if (k != remove) {
                                                        sibling->key[j] = node->key[k];
                                                        j++;
                                                }
                                        }
                                        for (j = sibling->children, k = 0; k < node->children; k++) {
                                                if (k != remove + 1) {
                                                        sibling->sub_ptr[j] = node->sub_ptr[k];
                                                        node->sub_ptr[k]->parent = sibling;
                                                        j++;
                                                }
                                        }
                                        sibling->children = j;
                                        /* delete merged node */
                                        sibling->next = node->next;
                                        non_leaf_delete(node);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i, level + 1);
                                }
                        } else {
                                /* remove key first in case of overflow during merging with sibling node */
                                for (; remove < node->children - 2; remove++) {
                                        node->key[remove] = node->key[remove + 1];
                                        node->sub_ptr[remove + 1] = node->sub_ptr[remove + 2];
                                }
                                node->children--;
                                if (sibling->children > (tree->order + 1) / 2) {
                                        /* parent key left rotation */
                                        node->key[node->children - 1] = parent->key[i + 1];
                                        parent->key[i + 1] = sibling->key[0];
                                        /* borrow the frist sub-node from right sibling */
                                        node->sub_ptr[node->children] = sibling->sub_ptr[0];
                                        sibling->sub_ptr[0]->parent = node;
                                        node->children++;
                                        /* left shift in right sibling */
                                        for (j = 0; j < sibling->children - 2; j++) {
                                                sibling->key[j] = sibling->key[j + 1];
                                        }
                                        for (j = 0; j < sibling->children - 1; j++) {
                                                sibling->sub_ptr[j] = sibling->sub_ptr[j + 1];
                                        }
                                        sibling->children--;
                                } else {
                                        /* move parent key down */
                                        node->key[node->children - 1] = parent->key[i + 1];
                                        node->children++;
                                        /* merge with right sibling */
                                        for (j = node->children - 1, k = 0; k < sibling->children - 1; j++, k++) {
                                                node->key[j] = sibling->key[k];
                                        }
                                        for (j = node->children - 1, k = 0; k < sibling->children; j++, k++) {
                                                node->sub_ptr[j] = sibling->sub_ptr[k];
                                                sibling->sub_ptr[k]->parent = node;
                                        }
                                        node->children = j;
                                        /* delete merged sibling */
                                        node->next = sibling->next;
                                        non_leaf_delete(sibling);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1, level + 1);
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
                                tree->head[level] = NULL;
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
        }
        node->children--;
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
                        /* find which sibling node with same parent to be borrowed from */
                        i = key_binary_search(parent->key, parent->children - 1, leaf->key[0]);
                        if (i >= 0) {
                                i = i + 1;
                                if (i == parent->children - 1) {
                                        /* the last node, no right sibling, choose left one */
                                        sibling = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        borrow = BORROW_FROM_LEFT;
                                } else {
                                        struct bplus_leaf *l_sib = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        struct bplus_leaf *r_sib = (struct bplus_leaf *)parent->sub_ptr[i + 1];
                                        /* if both left and right sibling found, choose the one with more entries */
                                        sibling = l_sib->entries >= r_sib->entries ? l_sib : r_sib;
                                        borrow = l_sib->entries >= r_sib->entries ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                                }
                        } else {
                                i = -i - 1;
                                if (i == 0) {
                                        /* the frist node, no left sibling, choose right one */
                                        sibling = (struct bplus_leaf *)parent->sub_ptr[i + 1];
                                        borrow = BORROW_FROM_RIGHT;
                                } else if (i == parent->children - 1) {
                                        /* the last node, no right sibling, choose left one */
                                        sibling = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        borrow = BORROW_FROM_LEFT;
                                } else {
                                        struct bplus_leaf *l_sib = (struct bplus_leaf *)parent->sub_ptr[i - 1];
                                        struct bplus_leaf *r_sib = (struct bplus_leaf *)parent->sub_ptr[i + 1];
                                        /* if both left and right sibling found, choose the one with more entries */
                                        sibling = l_sib->entries >= r_sib->entries ? l_sib : r_sib;
                                        borrow = l_sib->entries >= r_sib->entries ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                                }
                        }

                        /* locate parent node key to update later */
                        i = i - 1;

                        if (borrow == BORROW_FROM_LEFT) {
                                if (sibling->entries > (tree->entries + 1) / 2) {
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
                                        parent->key[i] = leaf->key[0];
                                } else {
                                        /* merge with left sibling */
                                        for (j = sibling->entries, k = 0; k < leaf->entries; k++) {
                                                if (k != remove) {
                                                        sibling->key[j] = leaf->key[k];
                                                        sibling->data[j] = leaf->data[k];
                                                        j++;
                                                }
                                        }
                                        sibling->entries = j;
                                        /* delete merged leaf */
                                        sibling->next = leaf->next;
                                        leaf_delete(leaf);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i, 1);
                                }
                        } else {
                                /* remove element first in case of overflow during merging with sibling node */
                                for (; remove < leaf->entries - 1; remove++) {
                                        leaf->key[remove] = leaf->key[remove + 1];
                                        leaf->data[remove] = leaf->data[remove + 1];
                                }
                                leaf->entries--;
                                if (sibling->entries > (tree->entries + 1) / 2) {
                                        /* borrow the first element from right sibling */
                                        leaf->key[leaf->entries] = sibling->key[0];
                                        leaf->data[leaf->entries] = sibling->data[0];
                                        leaf->entries++;
                                        /* left shift in right sibling */
                                        for (j = 0; j < sibling->entries - 1; j++) {
                                                sibling->key[j] = sibling->key[j + 1];
                                                sibling->data[j] = sibling->data[j + 1];
                                        }
                                        sibling->entries--;
                                        /* update parent key */
                                        parent->key[i + 1] = sibling->key[0];
                                } else {
                                        /* merge with right sibling */
                                        for (j = leaf->entries, k = 0; k < sibling->entries; j++, k++) {
                                                leaf->key[j] = sibling->key[k];
                                                leaf->data[j] = sibling->data[k];
                                        }
                                        leaf->entries = j;
                                        /* delete right sibling */
                                        leaf->next = sibling->next;
                                        leaf_delete(sibling);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i + 1, 1);
                                }
                        }
                        /* deletion finishes */
                        return 0;
                } else {
                        if (leaf->entries == 1) {
                                /* delete the only last node */
                                assert(key == leaf->key[0]);
                                tree->root = NULL;
                                tree->head[0] = NULL;
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

void
bplus_tree_dump(struct bplus_tree *tree)
{
        int i, j;

        for (i = tree->level - 1; i > 0; i--) {
                struct bplus_non_leaf *node = (struct bplus_non_leaf *)tree->head[i];
                if (node != NULL) {
                        printf("LEVEL %d:\n", i);
                        while (node != NULL) {
                                printf("node: ");
                                for (j = 0; j < node->children - 1; j++) {
                                        printf("%d ", node->key[j]);
                                }
                                printf("\n");
                                node = node->next;
                        }
                }
        }

        struct bplus_leaf *leaf = (struct bplus_leaf *)tree->head[0];
        if (leaf != NULL) {
                printf("LEVEL 0:\n");
                while (leaf != NULL) {
                        printf("leaf: ");
                        for (j = 0; j < leaf->entries; j++) {
                                printf("%d ", leaf->key[j]);
                        }
                        printf("\n");
                        leaf = leaf->next;
                }
        } else {
                printf("Empty tree!\n");
        }
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
bplus_tree_init(int level, int order, int entries)
{
        /* The max order of non leaf nodes must be more than two */
        assert(MAX_ORDER > MIN_ORDER);
        assert(level <= MAX_LEVEL && order <= MAX_ORDER && entries <= MAX_ENTRIES);

        struct bplus_tree *tree = malloc(sizeof(*tree));
        if (tree != NULL) {
                tree->root = NULL;
                tree->level = level;
                tree->order = order;
                tree->entries = entries;
                memset(tree->head, 0, MAX_LEVEL * sizeof(struct bplus_node *));
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
                                    ln = ln->next;
                            }
                    }
                    while (ln != NULL && ln->key[i] <= max) {
                            data = ln->data[i];
                            if (++i >= ln->entries) {
                                    ln = ln->next;
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
