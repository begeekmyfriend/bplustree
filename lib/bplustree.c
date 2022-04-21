#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include "bplustree.h"


enum
{
    BPLUS_TREE_LEAF,
    BPLUS_TREE_NONLEAF = 1,
};
enum
{
    LEFT_SIBLING,
    RIGHT_SIBLING = 1,
}


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
static  struct bplus_non_leaf *non_leaf_new()
{
    struct bplus_non_leaf *parent = calloc(1,sizeof(*parent));
    assert(parent!=NULL);
    list_init(&parent->link);
    parent->type = BPLUS_TREE_NONLEAF;
    parent->parent_key_idx = -1;
    return parent;
}

static key_t non_leaf_split_left(struct bplus_non_leaf *node,struct bplus_non_leaf *left,struct bplus_node *l_ch,
struct bplus_node *r_ch,key_t key,int insert)
{
    int i,j;
    int order = node->children;
    key_t split_key;
    int split = (order+1)/2;
   // insert sibling after parent node
    __list_add(&left->link,node->link.prev,&node->link);
    
    //list_add(&left->link,&node->link);
    
    for(i=0,j=0;i<split;i++,j++)
    {
        if(j==insert)
        {
            left->sub_ptr[j] == l_ch;
            left->sub_ptr[j]->parent = left;
            left->sub_ptr[j]->parent_key_idx = j-1;
            left->sub_ptr[j+1] = r_ch;
            left->sub_ptr[j+1]->parent = left;
            left->sub_ptr[j+1]->parent_key_idx = j; 
            j++;    
        }
        else
        {
            left->sub_ptr[j] = node->sub_ptr[i];
            left->sub_ptr[j]->parent = left;
            left->sub_ptr[j]->parent_key_idx = j-1;
        }
    }
    left->children = split;
     
    for(i=0,j=0;i<split-1;j++)
    {
        if(j==insert)
        {
            left->key[j] = key;
        }
        else
        {
            left->key[j] = node->key[i];
            i++;
        }
    }
    if(insert == split-1)
    {
        left->key[insert] = key;
        left->sub_ptr[insert] = l_ch;
        left->sub_ptr[insert]->parent = left;
        left->sub_ptr[insert]->parent_key_idx = j-1;
        node->sub_ptr[0] = r_ch;
        split_key = key;
        
    }
    else
    {
        node->sub_ptr[0] = node->sub_ptr[split-1];
        split_key = node->key[split-2];
        
    }
    
    node->sub_ptr[0]->parent = node;
    node->sub_ptr[0]->parent_key_idx = -1;
    
    for(i=split-1,j=0;i<order-1;i++,j++)
    {
        node->key[j] = node->sub_ptr[i+1];
        node->sub_ptr[j+1] = node->sub_ptr[i+1];
        node->sub_ptr[j+1]->parent = node;
        node->sub_ptr[j+1]->parent_key_idx = j;
    }
    node->sub_ptr[j] = node->sub_ptr[i];
    node->children = j+1;
    return split_key;
    

    

    
    
}
static key_t  non_leaf_split_right1(struct bplus_non_leaf *node,struct bplus_non_leaf *right,struct bplus_node *l_ch,
struct bplus_node *r_ch,key_t key,int insert)
{
    int i,j;
    int order = node->children;
    int split = (order+1)/2;
    key_t split_key;
    list_add(&right->link,&node->link);
    split_key = node->key[split-1];
    node->children = split;
    right->key[0] = key;
    right->sub_ptr[0] = l_ch;
    right->sub_ptr[0]->parent = right;
    right->sub_ptr[0]->parent_key_idx = -1;
    right->sub_ptr[1] = r_ch;
    right->sub_ptr[1]->parent = right;
    right->sub_ptr[1]->parent_key_idx = 0;

    for(i=split,j=1;i<order-1;i++,j++)
    {
        right->key[j] = node->key[i];
        right->sub_ptr[j+1] = node->sub_ptr[i+1];
        right->sub_ptr[j+1]->parent = right;
        right->sub_ptr[j+1]->parent_key_idx = j;        
    }
    right->children = j+1;
    return split_key;
}
static key_t  non_leaf_split_right2(struct bplus_non_leaf *node,struct bplus_non_leaf *right,struct bplus_node *l_ch,
struct bplus_node *r_ch,key_t key,int insert)
{
    int i,j;
    int order = node->children;
    int split = (order+1)/2;
    node->children = split+1;
    list_add(&right->link,&node->link);
    key_t split_key = node->key[split];
    
    right->sub_ptr[0] = node->sub_ptr[split+1];
    right->sub_ptr[0]->parent = right;
    right->sub_ptr[0]->parent_key_idx = -1;

    for(i=split+1,j=0;i<order-1,j++)
    {
        if(j!=insert-(split+1))
        {
            right->key[j] = node->key[i];
            right->sub_ptr[j+1] = node->sub_ptr[i+1];
            right->sub_ptr[j+1]->parent = right;
            right->sub_ptr[j+1]->parent_key_idx = j;
            i++;
        }
        
    }
    
    if(j>insert-split-1)
    {
        right->children = j+1;
    }
    else
    {
        assert(j==insert-split-1);
        right->children = j+2;
    }
    
    
    j = insert-split-1;
    right->key[j] = key;
    right->sub_ptr[j] = l_ch;
    right->sub_ptr[j]->parent = right;
    right->sub_ptr[j]->parent_key_idx = j-1;
    right->sub_ptr[j+1] = r_ch;
    right->sub_ptr[j+1]->parent = right;
    right->sub_ptr[j+1]->parent_key_idx = j;
    return split_key;
    
}
 
static void  non_leaf_simple_insert(struct bplus_non_leaf *node,struct bplus_node *l_ch,
struct bplus_node*r_ch,key_t key,int insert)
{
        int i;
        for (i = node->children - 1; i > insert; i--) {
                node->key[i] = node->key[i - 1];
                node->sub_ptr[i + 1] = node->sub_ptr[i];
                node->sub_ptr[i + 1]->parent_key_idx = i;
        }
        node->key[i] = key;
        node->sub_ptr[i] = l_ch;
        node->sub_ptr[i]->parent_key_idx = i - 1;
        node->sub_ptr[i + 1] = r_ch;
        node->sub_ptr[i + 1]->parent_key_idx = i;
        node->children++;
}

static int non_leaf_insert(struct bplus_tree *tree,struct bplus_non_leaf *node,struct bplus_node *l_ch,
struct bplus_node*r_ch,key_t key,int level)

{
    int insert = key_binary_search(node->key,node->children,key);
    assert(insert<0);
    insert = -insert-1;
    
    if(node->children == tree->order)
    {
        key_t split_key;
        int split = (node->children+1)/2;
        struct bplus_non_leaf *sibling = non_leaf_new();
        if(insert<split)
        {
            split_key = non_leaf_split_left(node,sibling,l_ch,r_ch,key,insert);
        }
        else if(insert == split)
        {
            split_key = non_leaf_split_right1(node,sibling,l_ch,r_ch,key,insert);
        }
        else
        {
            split_key = non_leaf_split_right2(node,sibling,l_ch,r_ch,key,insert);
        }
        
        if(insert<split)
        {
            return parent_node_build(tree,(struct bplus_node *)sibling,((struct bplus_node *)node,split_key,level));
            
        }
        else
        {
            return parent_node_build(tree,(struct bplus_node *)node,(struct bplus_node *)sibling,split_key,level);
        }    
    }
    else
    {
      non_leaf_simple_insert(node,l_ch,r_ch,key,insert);
    }
    return 0;
    
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


static int leaf_simple_insert(struct bplus_leaf *leaf,key_t key,int data, int insert)
{
    int i;
    for(i = leaf->entries;i>insert,i--)
    {
        leaf->key[i] = leaf->key[i-1];
        leaf->data[i] = leaf->data[i-1];
    }
    leaf->key[i] = key;
    leaf->data[i] = data;
    leaf->entries++;
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
        return leaf_simple_insert(leafnode,key_t key,int data,int insert);
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
  * if the tree is not empty: /
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
    // tree empty,initial root node
    struct bplus_leaf *root = leaf_new();
    root->key[0] = key;
    root->data[0] = data;
    root->entries = 1;
    tree->root =(struct bplus_non_leaf *) root;
    list_add(&root,&tree->list[tree->level]);
    return 0;
}
static void leaf_simple_remove(struct bplus_leaf *leafnode,int remove)
{
    
}
static int leaf_sibling_selelct()
static int leaf_remove(struct bplus_tree *tree,struct bplus_leaf *leafnode,key_t key)
{
    int remove = key_binary_search(leafnode->key,leaf->entries,key);
    if(remove<0) 
    {
        return -1;
    }
    if(leafnode->entries <= (tree->entries+1)/2)
    {
        struct bplus_non_leaf *parent = leafnode->parent;
        struct bplus_leaf *l_sib = list_prev_entry(leafnode,link);

        struct  bplus_leaf *r_sib = list_next_entry(leafnode,link);
        if(parent != NULL)
        {
            int i = leafnode->parent_key_idx;
            if(leaf_sibling_selelct(l_sib,r_sib,parent,i) == LEFT_SIBLING)
            {
                if(l_sib->entries > (tree->entries+1)/2)
                {
                    leaf_shift_from_left(left,l_sib,r_sib,i,remove);
                }
                else
                {
                    leaf_merge_into_left()
                }
            }
        }
           
    }
    else
    {
        leaf_simple_remove(leafnode,remove);
    }
    
}


static int bplus_tree_delete(struct bplus_tree *tree,key_t key)
{
    struct bplus_node *node = tree->root;
    while(node!=NULL)
    {
        if(is_leaf(node))
        {
            return leaf_remove(tree,(struct bplus_leaf*)node,key);    
        }
        else
        {
            struct bplus_non_leaf *nonleaf = (struct bplus_non_leaf *)nonleaf;
            int i = key_binary_search(nonleaf->key,nonleaf->children-1,key);
            if(i>=0)
            {
                node = nonleaf->sub_ptr[i+1];
            }
            else
            {
                i = -i-1;
                node = nonleaf->sub_ptr[i];
            }
        }

        
    }
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
    assert(BPLUS_MAX_ORDER>BPLUS_MIN_ORDER);
    assert(order<= BPLUS_MAX_ORDER && order>= BPLUS_MIN_ORDER);
    
    int i;
    struct bplus_tree *tree  = calloc(1,sizeof(*tree));
    if(tree != NULL)
    {
        tree->root = NULL;
        tree->entries = entries;
        tree->order = order;
        for(int i=0;i<BPLUS_MAX_LEVEL;i++)
        {
            list_init(&tree->list[i]);
        }
    }
    return tree;
}
