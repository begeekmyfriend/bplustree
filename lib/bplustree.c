#include<stdio.h>
#include<stdlib.h>
#include "bplustree.h"

enum
{
    BPLUS_TREE_LEAF,
    BPLUS_TREE_NONLEAF = 1,
};


int bplus_tree_get(struct bplus_tree *tree, key_t key)
{
    
}

/** 
 * 
 * */
int leaf_insert(struct bplus_tree *tree,struct bplus_leaf *leafnode,key_t key,int data)
{
    
}
/** 
 * 
 * */
static key_t key_binary_search(key_t *arr,int len,int target)
{
       
}

static struct bplus_leaf *leaf_new(void)
{

}


static inline int is_leaf(struct bplus_node *node)
{
    return node->type;
}

 /** insert key-value pair to tree.
  * if the tree is empty,we insert the key-value pair into root
  * 
  * if the tree is not empty: 
  *     leaf node: find the location with binary search and insert
  *     nonleaf node find suitable parent node, and insert it
  *     
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
