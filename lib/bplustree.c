#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include "bplustree.h"


enum
{
    BPLUS_TREE_LEAF,
    BPLUS_TREE_NONLEAF = 1,
};


int bplus_tree_get(struct bplus_tree *tree, key_t key)
{
    
}


static struct bplus_leaf *leaf_new(void)
{
    struct  bplus_non_leaf *node = calloc(1,sizeof(*node));
    assert(node!=NULL);
    list_init(&node->link);
    node->type = BPLUS_TREE_LEAF;
    node->parent_key_idx = -1;
    return node;

}

static int non_leaf_insert(struct bplus_tree *tree,struct bplus_non_leaf *node,struct bplus_node *l_ch,
struct bplus_node*r_ch,key_t key,int level)
{
    int insert = key_binary_search(node->key,node->children,key);
    assert(insert<0);
    insert = -insert-1;

    
    
}

static  struct bplus_non_leaf *non_leaf_new()
{
    struct bplus_non_leaf *parent = calloc(1,sizeof(*parent));
    assert(parent!=NULL);
    list_init(&parent->link);
    parent->type = BPLUS_TREE_NONLEAF;
    parent->parent_key_idx = -1;
    return parent;
}


static int parent_node_build(struct bplus_tree *tree,struct bplus_node *left,struct bplus_node *right,key_t key,int level)
{
    if(left->parent == NULL && right->parent == NULL)
    {
        struct bplus_non_leaf *parent = non_leaf_new();
        parent->key[0] = key;
        parent->sub_ptr[0] = left;
        parent->sub_ptr[0]->parent = parent;
        parent->sub_ptr[0]->parent_key_idx = -1;
        parent -> sub_ptr[1] = right;
        parent->sub_ptr[1]->parent = parent;
        parent -> sub_ptr[1]->parent_key_idx = 0;
        parent->children = 2;
        
        tree->root = parent;
        list_add(&parent->link,&tree->list[++tree->level]);
        return 0;
    }
    else if(right->parent == NULL)
    {
        right->parent = left->parent;
        return non_leaf_insert(tree,left->parent,left,right,key,level);
    }
    else
    {
        return non_leaf_insert(tree,left->parent,left,right,key,level);
    }
}


static int leaf_simple_insert()
{
    
}
/** copy orginal leaf key-value and  key-value which need to insert to new node(left)
 * 
 * 
 * 
 * */
static void leaf_split_left(struct bplus_leaf *leaf,struct bplus_leaf *left,key_t key,int data,int insert)
{
    int i,j;
    int split = (leaf->entries+1)/2;
    __list_add(&left->link,leaf->link.prev,&leaf->link);
    for(i=0,j=0;i<split-1;j++)
    {
        if(j == insert)
        {
            left->key[j] = key;
            left->data[j] = data;
        }
        else
        {
            left->key[j] = leaf->key[i];
            left->data[j] = leaf->data[j];
            i++;
        }
    }
    // split == insert add tail
    if(j == insert)
    {
            left->key[j] = key;
            left->data[j] = data;        
    }
    left->entries = j;
    // left shift for right half node, i == split
    for(j=0;i<leaf->entries;i++,j++)
    {
        leaf->key[j] = leaf->key[i];
        leaf->data[j] = leaf->data[i];
        
    }
    leaf->entries = j; 
}

static void leaf_split_right(struct bplus_leaf *leaf,struct bplus_leaf *right,key_t key,int data,int insert)
{
    int i,j;
    int split = (leaf->entries+1)/2;
    //__list_add(&right->link,leaf->link.prev,leaf->link);

    list_add(&right->link,&leaf->link);
    // key[split,entries-1] -- key(right)[0,entries-split-1]
    for(i=split,j=0;i<leaf->entries;j++)
    {
        if(j!=insert-split)
        {
            right->key[j] = leaf->key[i];
            right->data[j] = leaf->data[i];
            i++;
        }
    }
    
    if(j>insert -split)
    {
        right->entries = j;
    }
    // j = insert - split means insert key is largest(at the rightest of right split node)
    else
    {
        assert(j == (insert -split));
        right->entries = j+1;
    }
    
    j = insert - split;
    right->key[j] = key;
    right->data[j] = data;
    leaf->entries = split;

}

/** insert value into leaf
 * @return 0 if we succeed insert.  -1 means node is already exist.
 * 
 */
int leaf_insert(struct bplus_tree *tree,struct bplus_leaf *leafnode,key_t key,int data)
{
    int insert = key_binary_search(leafnode->key,leafnode->entries,key);
    if(insert>=0) return -1;
    // insert is leafnode->entries
    insert = -insert-1;
    // node is full,we need to split
    if(leafnode->entries == tree->entries)
    {
        int split = (tree->entries+1)/2;
        struct bplus_leaf *sibling = leaf_new();
        // 分裂左半边节点
        if(insert<split) 
        {
            leaf_split_left(leafnode,sibling,key,data,insert);
        }
        else
        {
            leaf_split_right(leafnode,sibling,key,data,insert);
        }

        if(insert< split)
        {
            return parent_node_build(tree,(struct bplues_node *)sibling,(struct bplues_node *)leafnode,leafnode->key[0],0);
        }
        else
        {
            return parent_node_build(tree,(struct bplues_node *)leafnode,(struct bplues_node *)sibling,leafnode->key[0],0);
        }
    }
    else
    {
        return leaf_simple_insert();
    }
    return 0;
    
    
}
/** find the key in key array with binary search
 * @return insert location in sorted array. -len-1 if we do not find.
 * */
static key_t key_binary_search(key_t *arr,int len,int target)
{
    int low = 0;
    int high = len-1;
    while(low<=high)
    {
        int mid = low+(high-low)/2; 
        if(arr[mid]>target)
        {
            low = mid+1;
        }
        else if(arr[mid<target])
        {
            high = mid-1;
        }
        // arr[mid] = target
        else 
        {
            return mid;
        }
    }
    // low > high
    return -high-1;
}



/** whether node is leaf or not
 * 
 * @return 1 if  BPLUS_TREE_LEAF
 * */
static inline int is_leaf(struct bplus_node *node)
{
    return node->type == BPLUS_TREE_LEAF;
}
/** insert key-value pair to tree.
  * if the tree is empty,we insert the key-value pair into root
  * 
  * if the tree is not empty: 
  *     leaf node: find the location with binary search and insert
  *     nonleaf node find suitable parent node, and insert it
  * 
  * Perform a search to determine what bucket the new record should go into.
    If the bucket is not full (at most {\displaystyle b-1}b-1 entries after the insertion), add the record.
    Otherwise, before inserting the new record
    split the bucket.
    original node has ⎡(L+1)/2⎤ items
    new node has ⎣(L+1)/2⎦ items
    Move ⎡(L+1)/2⎤-th key to the parent, and insert the new node to the parent.
    Repeat until a parent is found that need not split.
    If the root splits, treat it as if it has an empty parent and split as outline above. 
  * 
   * @return 0 if insert succeed , -1 if delete succeed
   * @param tree pointer to bplus tree
   * @param key  key in key-value pair
   * @param data value in key-value pair
   
*/
static int bplus_tree_insert(struct bplus_tree *tree,key_t key,int data)
{
    struct bplus_node *node = tree->root;
    // bplus tree exist
    while(node!=NULL)
    {
        if(is_leaf(node))
        {
            struct bplus_leaf *leafnode =  ( struct bplus_leaf *)node;
            return leaf_insert(tree,leafnode,key,data);
        }
        else
        {
            struct bplus_non_leaf *non_leafnode = (struct bplus_non_leaf *)node;
            int i = key_binary_search(non_leafnode->key,non_leafnode->children-1,key);
            if(i>=0)
            {
                node = non_leafnode->sub_ptr[i+1];
            }
            {
                i = -i-1;
                node = non_leafnode->sub_ptr[i];// insert at tail
            }
            
        }
        
    }
    // tree empty
    struct bplus_leaf *root = leaf_new();
    root->key[0] = key;
    root->data[0] = data;
    root->entries = 1;
    tree->root =(struct bplus_non_leaf *) root;
    list_add(&root,&tree->list[tree->level]);
    return 0;
    
    

}
static int bplus_tree_delete(struct bplus_tree *tree,key_t key)
{
    
}

int bplus_tree_put(struct bplus_tree *tree, key_t key, int data)
{
    if(data) 
    {
        return bplus_tree_insert(tree,key,data);
    } 
    else
    {
        return bplus_tree_delete(tree,key);
    }
}

struct bplus_tree *bplus_tree_init(int order, int entries)
{
    
}
