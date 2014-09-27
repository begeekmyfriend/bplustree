/********************************************
*
* License: MIT
*
* Date: 09/27/14
*
* Author: Leo Ma <begeekmyfriend@gmail.com>
*
********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define DEGREE        7
#define ENTRIES       10
#define MAX_LEVEL     5

enum {
        BPLUS_TREE_LEAF,
        BPLUS_TREE_NON_LEAF = 1,
};

enum {
        BORROW_FROM_LEFT,
        BORROW_FROM_RIGHT = 1,
};

struct node {
        int type;
        struct non_leaf *parent;
};

struct non_leaf {
        int type;
        struct non_leaf *parent;
        struct non_leaf *next;
        int children;
        int key[DEGREE - 1];
        struct node *sub_ptr[DEGREE];
};

struct leaf {
        int type;
        struct non_leaf *parent;
        struct leaf *next;
        int entries;
        int key[ENTRIES];
        int data[ENTRIES];
};

struct tree {
        struct node *root;
        struct node *head[MAX_LEVEL];
};

static struct tree *bplus_tree;

static int
key_binary_search(int *arr, int len, int target)
{
        int low = -1;
        int high = len;
        while (low + 1 < high) {
                int mid = low + (high - low) / 2;
                if (target < arr[mid]) {
                        high = mid;
                } else {
                        low = mid;
                }
        }
        if (low < 0 || arr[low] != target) {
                return -low - 2;
        } else {
                return low;
        }
}

static struct non_leaf *
non_leaf_new()
{
        struct non_leaf *node = malloc(sizeof(*node));
        assert(node != NULL);
        node->type = BPLUS_TREE_NON_LEAF;
        memset(node->key, 0, DEGREE * sizeof(int));
        memset(node->sub_ptr, 0, DEGREE * sizeof(struct node *));
        node->parent = NULL;
        node->next = NULL;
        node->children = 0;
        return node;
}

static struct leaf *
leaf_new()
{
        struct leaf *node = malloc(sizeof(*node));
        assert(node != NULL);
        node->type = BPLUS_TREE_LEAF;
        memset(node->key, 0, ENTRIES * sizeof(int));
        memset(node->data, 0, ENTRIES * sizeof(int));
        node->parent = NULL;
        node->next = NULL;
        node->entries = 0;
        return node;
}

static void
non_leaf_delete(struct non_leaf *node)
{
        free(node);
}

static void
leaf_delete(struct leaf *node)
{
        free(node);
}

int
bplus_tree_search(struct tree *tree, int key)
{
        int i;
        struct node *node = tree->root;
        struct non_leaf *nln;
        struct leaf *ln;

        while (node != NULL) {
                switch (node->type) {
                        case BPLUS_TREE_NON_LEAF:
                                nln = (struct non_leaf *)node;
                                i = key_binary_search(nln->key, nln->children - 1, key);
                                if (i >= 0) {
                                        node = nln->sub_ptr[i + 1];
                                } else {
                                        i = -i - 1;
                                        node = nln->sub_ptr[i];
                                }
                                break;
                        case BPLUS_TREE_LEAF:
                                ln = (struct leaf *)node;
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

static void
non_leaf_insert(struct tree *tree, struct non_leaf *node, struct node *sub_node, int key, int level)
{
        int i, j, split_key;
        int split = 0;
        struct non_leaf *sibling;

        int insert = key_binary_search(node->key, node->children - 1, key);
        assert(insert < 0);
        insert = -insert - 1;

        /* node full */
        if (node->children == DEGREE) {
                /* split key index */
                split = (DEGREE + 1) / 2;
                /* splited sibling node */
                sibling = non_leaf_new();
                sibling->next = node->next;
                node->next = sibling;
                node->children = split + 1;
                /* sibling node replication */
                if (insert < split) {
                        i = split - 1, j = 0;
                        split_key = node->key[i];
                        sibling->sub_ptr[j] = node->sub_ptr[i + 1];
                        node->sub_ptr[i + 1]->parent = sibling;
                        i++;
                        for (; i < DEGREE - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                node->sub_ptr[i + 1]->parent = sibling;
                        }
                        sibling->children = j + 1;
                        /* node insertion and its children count stays unchanged(split + 1) */
                        for (i = node->children - 1; i > insert; i--) {
                                node->key[i] = node->key[i - 1];
                                node->sub_ptr[i + 1] = node->sub_ptr[i];
                        }
                        node->key[i] = key;
                        node->sub_ptr[i + 1] = sub_node;
                } else if (insert == split) {
                        i = split, j = 0;
                        split_key = key;
                        sibling->sub_ptr[j] = sub_node;
                        sub_node->parent = sibling;
                        i++;
                        for (; i < DEGREE - 1; i++, j++) {
                                sibling->key[j] = node->key[i];
                                sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                node->sub_ptr[i + 1]->parent = sibling;
                        }
                        sibling->children = j + 1;
                } else {
                        i = split, j = 0;
                        split_key = node->key[i];
                        sibling->sub_ptr[j] = node->sub_ptr[i + 1];
                        node->sub_ptr[i + 1]->parent = sibling;
                        i++;
                        while (i < DEGREE - 1) {
                                if (j != insert - split) {
                                        sibling->key[j] = node->key[i];
                                        sibling->sub_ptr[j + 1] = node->sub_ptr[i + 1];
                                        node->sub_ptr[i + 1]->parent = sibling;
                                        i++;
                                }
                                j++;
                        }
                        /* sibling node children count */
                        if (j > insert - split) {
                                sibling->children = j + 1;
                        } else {
                                sibling->children = insert - split + 1;
                        }
                        /* insert new key */
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
                struct non_leaf *parent = node->parent;
                if (parent == NULL) {
                        /* new parent */
                        parent = non_leaf_new();
                        parent->key[0] = split_key;
                        parent->sub_ptr[0] = (struct node *)node;
                        parent->sub_ptr[1] = (struct node *)sibling;
                        parent->children = 2;
                        /* new root */
                        tree->root = (struct node *)parent;
                        if (++level >= MAX_LEVEL) {
                                fprintf(stderr, "!!Panic: Level exceeded, please expand the non-leaf degree or leaf capacity of the tree!\n");
                                assert(0);
                        }
                        tree->head[level] = (struct node *)parent;
                        node->parent = parent;
                        sibling->parent = parent;
                } else {
                        /* Trace upwards */
                        sibling->parent = parent;
                        non_leaf_insert(tree, parent, (struct node *)sibling, split_key, level + 1);
                }
        }
}

static void
leaf_insert(struct tree *tree, struct leaf *leaf, int key, int data)
{
        int i, j, split = 0;
        struct leaf *sibling;

        int insert = key_binary_search(leaf->key, leaf->entries, key);
        if (insert >= 0) {
                /* Already exists */
                return;
        }
        insert = -insert - 1;

        /* node full */
        if (leaf->entries == ENTRIES) {
                /* split = [m/2] */
                split = (ENTRIES + 1) / 2;
                /* splited sibling node */
                sibling = leaf_new();
                sibling->next = leaf->next;
                leaf->next = sibling;
                leaf->entries = split;
                /* sibling leaf replication */
                if (insert < split) {
                        for (i = split - 1, j = 0; i < ENTRIES; i++, j++) {
                                sibling->key[j] = leaf->key[i];
                                sibling->data[j] = leaf->data[i];
                        }
                        sibling->entries = j;
                        /* leaf insertion and its entry count stays unchanged(split + 1) */
                        for (i = leaf->entries; i > insert; i--) {
                                leaf->key[i] = leaf->key[i - 1];
                                leaf->data[i] = leaf->data[i - 1];
                        }
                        leaf->key[i] = key;
                        leaf->data[i] = data;
                } else {
                        i = split, j = 0;
                        while (i < ENTRIES) {
                                if (j != insert - split) {
                                        sibling->key[j] = leaf->key[i];
                                        sibling->data[j] = leaf->data[i];
                                        i++;
                                }
                                j++;
                        }
                        /* sibling leaf entries */
                        if (j > insert - split) {
                                sibling->entries = j;
                        } else {
                                sibling->entries = insert - split + 1;
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
                struct non_leaf *parent = leaf->parent;
                if (parent == NULL) {
                        parent = non_leaf_new();
                        parent->key[0] = sibling->key[0];
                        parent->sub_ptr[0] = (struct node *)leaf;
                        parent->sub_ptr[1] = (struct node *)sibling;
                        parent->children = 2;
                        /* new root */
                        tree->root = (struct node *)parent;
                        tree->head[1] = (struct node *)parent;
                        leaf->parent = parent;
                        sibling->parent = parent;
                } else {
                        /* trace upwards */
                        sibling->parent = parent;
                        non_leaf_insert(tree, parent, (struct node *)sibling, sibling->key[0], 1);
                }
        }
}

void
bplus_tree_insert(struct tree *tree, int key, int data)
{
        int i;
        struct node *node = tree->root;
        struct non_leaf *nln;
        struct leaf *ln;

        if (node == NULL) {
                struct leaf *leaf = leaf_new();
                leaf->key[0] = key;
                leaf->data[0] = data;
                leaf->entries = 1;
                tree->head[0] = (struct node *)leaf;
                tree->root = (struct node *)leaf;
                return;
        }

        while (node != NULL) {
                switch (node->type) {
                        case BPLUS_TREE_NON_LEAF:
                                nln = (struct non_leaf *)node;
                                i = key_binary_search(nln->key, nln->children - 1, key);
                                if (i >= 0) {
                                        node = nln->sub_ptr[i + 1];
                                } else {
                                        i = -i - 1;
                                        node = nln->sub_ptr[i];
                                }
                                break;
                        case BPLUS_TREE_LEAF:
                                ln = (struct leaf *)node;
                                leaf_insert(tree, ln, key, data);
                                return;
                        default:
                                break;
                }
        }
}

static void
non_leaf_remove(struct tree *tree, struct non_leaf *node, int remove, int level)
{
        int i, j, k;
        struct non_leaf *sibling;

        if (node->children <= (DEGREE + 1) / 2) {
                struct non_leaf *parent = node->parent;
                if (parent != NULL) {
                        int borrow = 0;
                        /* find which sibling node with same parent to be borrowed from */
                        i = key_binary_search(parent->key, parent->children - 1, node->key[0]);
                        assert(i < 0);
                        i = -i - 1;
                        if (i == 0) {
                                /* no left sibling, choose right one */
                                sibling = (struct non_leaf *)parent->sub_ptr[i + 1];
                                borrow = BORROW_FROM_RIGHT;
                        } else if (i == parent->children - 1) {
                                /* no right sibling, choose left one */
                                sibling = (struct non_leaf *)parent->sub_ptr[i - 1];
                                borrow = BORROW_FROM_LEFT;
                        } else {
                                struct non_leaf *l_sib = (struct non_leaf *)parent->sub_ptr[i - 1];
                                struct non_leaf *r_sib = (struct non_leaf *)parent->sub_ptr[i + 1];
                                /* if both left and right sibling found, choose the one with more children */
                                sibling = l_sib->children >= r_sib->children ? l_sib : r_sib;
                                borrow = l_sib->children >= r_sib->children ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                        }

                        /* locate parent node key index */
                        if (i > 0) {
                                i = i - 1;
                        }

                        if (borrow == BORROW_FROM_LEFT) {
                                if (sibling->children > (DEGREE + 1) / 2) {
                                        /* node right shift */
                                        for (j = remove; j > 0; j--) {
                                                node->key[j] = node->key[j - 1];
                                        }
                                        for (j = remove + 1; j > 0; j--) {
                                                node->sub_ptr[j] = node->sub_ptr[j - 1];
                                        }
                                        /* right rotate key */
                                        node->key[0] = parent->key[i];
                                        parent->key[i] = sibling->key[sibling->children - 2];
                                        /* move left sibling's last sub-node into node's first location */
                                        node->sub_ptr[0] = sibling->sub_ptr[sibling->children - 1];
                                        sibling->sub_ptr[sibling->children - 1]->parent = node;
                                        sibling->children--;
                                } else {
                                        /* move parent key down */
                                        sibling->key[sibling->children - 1] = parent->key[i];
                                        /* merge node and left sibling */
                                        for (j = sibling->children, k = 0; k < node->children - 1; k++) {
                                                if (k != remove) {
                                                        sibling->key[j] = node->key[k];
                                                        j++;
                                                }
                                        }
                                        for (j = sibling->children, k = 0; k < node->children - 1; k++) {
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
                                /* remove key first in case of overflow merging with sibling node */
                                for (; remove < node->children - 2; remove++) {
                                        node->key[remove] = node->key[remove + 1];
                                        node->sub_ptr[remove + 1] = node->sub_ptr[remove + 2];
                                }
                                node->children--;
                                if (sibling->children > (DEGREE + 1) / 2) {
                                        /* left rotate key */
                                        node->key[node->children - 1] = parent->key[i];
                                        parent->key[i] = sibling->key[0];
                                        node->children++;
                                        /* move right sibling's first sub-node into node's last location */
                                        node->sub_ptr[node->children - 1] = sibling->sub_ptr[0];
                                        sibling->sub_ptr[0]->parent = node;
                                        /* right sibling left shift */
                                        for (j = 0; j < sibling->children - 2; j++) {
                                                sibling->key[j] = sibling->key[j + 1];
                                        }
                                        for (j = 0; j < sibling->children - 1; j++) {
                                                sibling->sub_ptr[j] = sibling->sub_ptr[j + 1];
                                        }
                                        sibling->children--;
                                } else {
                                        /* move parent key down */
                                        node->key[node->children - 1] = parent->key[i];
                                        node->children++;
                                        /* merge node and right sibling */
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
                                        non_leaf_remove(tree, parent, i, level + 1);
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

static void
leaf_remove(struct tree *tree, struct leaf *leaf, int key)
{
        int i, j, k;
        struct leaf *sibling;

        int remove = key_binary_search(leaf->key, leaf->entries, key);
        if (remove < 0) {
                return;
        }

        if (leaf->entries <= (ENTRIES + 1) / 2) {
                struct non_leaf *parent = leaf->parent;
                if (parent != NULL) {
                        int borrow = 0;
                        /* find which sibling node with same parent to be borrowed from */
                        i = key_binary_search(parent->key, parent->children - 1, leaf->key[0]);
                        if (i >= 0) {
                                i = i + 1;
                                if (i == parent->children - 1) {
                                        /* the last node, no right sibling, choose left one */
                                        sibling = (struct leaf *)parent->sub_ptr[i - 1];
                                        borrow = BORROW_FROM_LEFT;
                                }  else {
                                        struct leaf *l_sib = (struct leaf *)parent->sub_ptr[i - 1];
                                        struct leaf *r_sib = (struct leaf *)parent->sub_ptr[i + 1];
                                        /* if both left and right sibling found, choose the one with more entries */
                                        sibling = l_sib->entries >= r_sib->entries ? l_sib : r_sib;
                                        borrow = l_sib->entries >= r_sib->entries ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                                }
                        } else {
                                i = -i - 1;
                                if (i == 0) {
                                        /* the frist node, no left sibling, choose right one */
                                        sibling = (struct leaf *)parent->sub_ptr[i + 1];
                                        borrow = BORROW_FROM_RIGHT;
                                } else if (i == parent->children - 1) {
                                        /* the last node, no right sibling, choose left one */
                                        sibling = (struct leaf *)parent->sub_ptr[i - 1];
                                        borrow = BORROW_FROM_LEFT;
                                } else {
                                        struct leaf *l_sib = (struct leaf *)parent->sub_ptr[i - 1];
                                        struct leaf *r_sib = (struct leaf *)parent->sub_ptr[i + 1];
                                        /* if both left and right sibling found, choose the one with more entries */
                                        sibling = l_sib->entries >= r_sib->entries ? l_sib : r_sib;
                                        borrow = l_sib->entries >= r_sib->entries ? BORROW_FROM_LEFT : BORROW_FROM_RIGHT;
                                }
                        }

                        /* locate parent node key index */
                        if (i > 0) {
                                i = i - 1;
                        }

                        if (borrow == BORROW_FROM_LEFT) {
                                if (sibling->entries > (ENTRIES + 1) / 2) {
                                        /* leaf node right shift */
                                        parent->key[i] = sibling->key[sibling->entries - 1];
                                        for (; remove > 0; remove--) {
                                                leaf->key[remove] = leaf->key[remove - 1];
                                                leaf->data[remove] = leaf->data[remove - 1];
                                        }
                                        leaf->key[0] = sibling->key[sibling->entries - 1];
                                        leaf->data[0] = sibling->data[sibling->entries - 1];
                                        sibling->entries--;
                                        /* adjust parent key */
                                        parent->key[i] = leaf->key[0];
                                } else {
                                        /* merge leaf and left sibling */
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
                                /* remove entry first in case of overflow merging with sibling node */
                                for (; remove < leaf->entries - 1; remove++) {
                                        leaf->key[remove] = leaf->key[remove + 1];
                                        leaf->data[remove] = leaf->data[remove + 1];
                                }
                                leaf->entries--;
                                if (sibling->entries > (ENTRIES + 1) / 2) {
                                        /* borrow */
                                        leaf->key[leaf->entries] = sibling->key[0];
                                        leaf->data[leaf->entries] = sibling->data[0];
                                        leaf->entries++;
                                        /* right sibling node left shift */
                                        for (j = 0; j < sibling->entries - 1; j++) {
                                                sibling->key[j] = sibling->key[j + 1];
                                                sibling->data[j] = sibling->data[j + 1];
                                        }
                                        sibling->entries--;
                                        /* adjust parent key */
                                        parent->key[i] = sibling->key[0];
                                } else {
                                        /* merge leaf node */
                                        for (j = leaf->entries, k = 0; k < sibling->entries; j++, k++) {
                                                leaf->key[j] = sibling->key[k];
                                                leaf->data[j] = sibling->data[k];
                                        }
                                        leaf->entries = j;
                                        /* delete merged sibling */
                                        leaf->next = sibling->next;
                                        leaf_delete(sibling);
                                        /* trace upwards */
                                        non_leaf_remove(tree, parent, i, 1);
                                }
                        }
                        /* deletion finishes */
                        return;
                } else {
                        if (leaf->entries == 1) {
                                /* delete the only last node */
                                assert(key == leaf->key[0]);
                                tree->root = NULL;
                                tree->head[0] = NULL;
                                leaf_delete(leaf);
                                return;
                        }
                }
        }

        /* simple deletion */
        for (; remove < leaf->entries - 1; remove++) {
                leaf->key[remove] = leaf->key[remove + 1];
                leaf->data[remove] = leaf->data[remove + 1];
        }
        leaf->entries--;
}

void
bplus_tree_delete(struct tree *tree, int key)
{
        int i;
        struct node *node = tree->root;
        struct non_leaf *nln;
        struct leaf *ln;

        while (node != NULL) {
                switch (node->type) {
                        case BPLUS_TREE_NON_LEAF:
                                nln = (struct non_leaf *)node;
                                i = key_binary_search(nln->key, nln->children - 1, key);
                                if (i >= 0) {
                                        node = nln->sub_ptr[i + 1];
                                } else {
                                        i = -i - 1;
                                        node = nln->sub_ptr[i];
                                }
                                break;
                        case BPLUS_TREE_LEAF:
                                ln = (struct leaf *)node;
                                leaf_remove(tree, ln, key);
                                return;
                        default:
                                break;
                }
        }
}

void
bplus_tree_dump(struct tree *tree)
{
        int i, j;

        for (i = MAX_LEVEL; i > 0; i--) {
                struct non_leaf *node = (struct non_leaf *)tree->head[i];
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

        struct leaf *leaf = (struct leaf *)tree->head[0];
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
get(int key)
{
        int data;
        data = bplus_tree_search(bplus_tree, key); 
        if (data) {
                return data;
        } else {
                return -1;
        }
}

void
put(int key, int data)
{
        if (data) {
                bplus_tree_insert(bplus_tree, key, data);
        } else {
                bplus_tree_delete(bplus_tree, key);
        }
}

void
init(void)
{
        /* The degree of non leaf nodes must be more than two */
        assert(DEGREE > 2);
        bplus_tree = malloc(sizeof(*bplus_tree));
        assert(bplus_tree != NULL);
        bplus_tree->root = NULL;
        memset(bplus_tree->head, 0, MAX_LEVEL * sizeof(struct node *));
}

int
main(void)
{
        init();
#if 0
        put(24, 24);
        put(72, 72);
        put(1, 1);
        put(39, 39);
        put(53, 53);
        put(63, 63);
        put(90, 90);
        put(88, 88);
        put(15, 15);
        put(10, 10);
        put(44, 44);
        put(68, 68);
        put(74, 74);
        bplus_tree_dump(bplus_tree);
#endif

#if 0
        put(10, 10);
        put(15, 15);
        put(18, 18);
        put(22, 22);
        put(27, 27);
        put(34, 34);
        put(40, 40);
        put(44, 44);
        put(47, 47);
        put(54, 54);
        put(67, 67);
        put(72, 72);
        put(74, 74);
        put(78, 78);
        put(81, 81);
        put(84, 84);
        bplus_tree_dump(bplus_tree);
#endif

#if 0
        printf("key:24 data:%d\n", get(24));
        printf("key:72 data:%d\n", get(72));
        printf("key:01 data:%d\n", get(1));
        printf("key:39 data:%d\n", get(39));
        printf("key:53 data:%d\n", get(53));
        printf("key:63 data:%d\n", get(63));
        printf("key:90 data:%d\n", get(90));
        printf("key:88 data:%d\n", get(88));
        printf("key:15 data:%d\n", get(15));
        printf("key:10 data:%d\n", get(10));
        printf("key:44 data:%d\n", get(44));
        printf("key:68 data:%d\n", get(68));
        printf("key:74 data:%d\n", get(74));
        printf("key:44 data:%d\n", get(44));
        printf("key:45 data:%d\n", get(45));
        printf("key:46 data:%d\n", get(46));
        printf("key:47 data:%d\n", get(47));
        printf("key:48 data:%d\n", get(48));
#endif
#if 0
        put(90, 0);
        bplus_tree_dump(bplus_tree);
        put(88, 0);
        bplus_tree_dump(bplus_tree);
        put(74, 0);
        bplus_tree_dump(bplus_tree);
        put(72, 0);
        bplus_tree_dump(bplus_tree);
        put(68, 0);
        bplus_tree_dump(bplus_tree);
        put(63, 0);
        bplus_tree_dump(bplus_tree);
        put(53, 0);
        bplus_tree_dump(bplus_tree);
        put(44, 0);
        bplus_tree_dump(bplus_tree);
        put(39, 0);
        bplus_tree_dump(bplus_tree);
        put(24, 0);
        bplus_tree_dump(bplus_tree);
        put(15, 0);
        bplus_tree_dump(bplus_tree);
        put(10, 0);
        bplus_tree_dump(bplus_tree);
        put(1, 0);
        bplus_tree_dump(bplus_tree);
#endif

#define MAX_KEY  100

        int i;

        for (i = 1; i <= MAX_KEY; i++) {
                put(i, i);
        }
        bplus_tree_dump(bplus_tree);

        while (--i > 0) {
                put(i, 0);
        }
        bplus_tree_dump(bplus_tree);

        for (i = MAX_KEY; i > 0; i--) {
                put(i, i);
        }
        bplus_tree_dump(bplus_tree);

        for (i = 1; i <= MAX_KEY; i++) {
                put(i, 0);
        }
        bplus_tree_dump(bplus_tree);

        return 0;
}
