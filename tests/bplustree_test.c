#include <stdio.h>
#include <stdlib.h>

#include "bplustree.h"

static void bplus_tree_get_put_test(struct bplus_tree *tree)
{
        int i;

        printf("\n> B+tree getter and setter testing...\n");

        bplus_tree_put(tree, 24, 24);
        bplus_tree_put(tree, 72, 72);
        bplus_tree_put(tree, 1, 1);
        bplus_tree_put(tree, 39, 39);
        bplus_tree_put(tree, 53, 53);
        bplus_tree_put(tree, 63, 63);
        bplus_tree_put(tree, 90, 90);
        bplus_tree_put(tree, 88, 88);
        bplus_tree_put(tree, 15, 15);
        bplus_tree_put(tree, 10, 10);
        bplus_tree_put(tree, 44, 44);
        bplus_tree_put(tree, 68, 68);
        bplus_tree_put(tree, 74, 74);
        bplus_tree_dump(tree);

        bplus_tree_put(tree, 10, 10);
        bplus_tree_put(tree, 15, 15);
        bplus_tree_put(tree, 18, 18);
        bplus_tree_put(tree, 22, 22);
        bplus_tree_put(tree, 27, 27);
        bplus_tree_put(tree, 34, 34);
        bplus_tree_put(tree, 40, 40);
        bplus_tree_put(tree, 44, 44);
        bplus_tree_put(tree, 47, 47);
        bplus_tree_put(tree, 54, 54);
        bplus_tree_put(tree, 67, 67);
        bplus_tree_put(tree, 72, 72);
        bplus_tree_put(tree, 74, 74);
        bplus_tree_put(tree, 78, 78);
        bplus_tree_put(tree, 81, 81);
        bplus_tree_put(tree, 84, 84);
        bplus_tree_dump(tree);

        printf("key:24 data index:%ld\n", bplus_tree_get(tree, 24));
        printf("key:72 data index:%ld\n", bplus_tree_get(tree, 72));
        printf("key:1  data index:%ld\n", bplus_tree_get(tree, 1));
        printf("key:39 data index:%ld\n", bplus_tree_get(tree, 39));
        printf("key:53 data index:%ld\n", bplus_tree_get(tree, 53));
        printf("key:63 data index:%ld\n", bplus_tree_get(tree, 63));
        printf("key:90 data index:%ld\n", bplus_tree_get(tree, 90));
        printf("key:88 data index:%ld\n", bplus_tree_get(tree, 88));
        printf("key:15 data index:%ld\n", bplus_tree_get(tree, 15));
        printf("key:10 data index:%ld\n", bplus_tree_get(tree, 10));
        printf("key:44 data index:%ld\n", bplus_tree_get(tree, 44));
        printf("key:68 data index:%ld\n", bplus_tree_get(tree, 68));

        /* Not found */
        printf("key:100 data index:%ld\n", bplus_tree_get(tree, 100));

        /* Clear all */
        printf("\n> Clear all...\n");
        for (i = 1; i <= 100; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Not found */
        printf("key:100 data index:%ld\n", bplus_tree_get(tree, 100));
}

static void bplus_tree_insert_delete_test(struct bplus_tree *tree)
{
        int i, max_key = 100;

        printf("\n> B+tree insertion and deletion testing...\n");

        /* Ordered insertion and deletion */
        printf("\n-- Insert 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        printf("\n-- Delete 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Ordered insertion and reversed deletion */
        printf("\n-- Insert 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        printf("\n-- Delete %d to 1, dump:\n", max_key);
        while (--i > 0) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Reversed insertion and ordered deletion */
        printf("\n-- Insert %d to 1, dump:\n", max_key);
        for (i = max_key; i > 0; i--) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        printf("\n-- Delete 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Reversed insertion and reversed deletion */
        printf("\n-- Insert %d to 1, dump:\n", max_key);
        for (i = max_key; i > 0; i--) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        printf("\n-- Delete %d to 1, dump:\n", max_key);
        for (i = max_key; i > 0; i--) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);
}

static void bplus_tree_test(void)
{
        printf("\n>>> B+tree general test.\n");

        /* Init b+tree */
        struct bplus_tree *tree = bplus_tree_init("/tmp/data.index", 128);
        if (tree == NULL) {
                fprintf(stderr, "Init failure!\n");
                exit(-1);
        }

        /* getter and setter test */
        bplus_tree_get_put_test(tree);

        /* insertion and deletion test */
        bplus_tree_insert_delete_test(tree);

        /* Deinit b+tree */
        bplus_tree_deinit(tree);
}

int main(void)
{
        bplus_tree_test();
        return 0;
}
