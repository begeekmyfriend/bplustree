#include <stdio.h>
#include <stdlib.h>

#include "bplustree.h"

struct bplus_tree_config {
        int order;
        int entries;
}; 

static void bplus_tree_get_put_test(struct bplus_tree *tree)
{
        int i;

        fprintf(stderr, "\n> B+tree getter and setter testing...\n");

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

        fprintf(stderr, "key:24 data:%d\n", bplus_tree_get(tree, 24));
        fprintf(stderr, "key:72 data:%d\n", bplus_tree_get(tree, 72));
        fprintf(stderr, "key:1 data:%d\n", bplus_tree_get(tree, 1));
        fprintf(stderr, "key:39 data:%d\n", bplus_tree_get(tree, 39));
        fprintf(stderr, "key:53 data:%d\n", bplus_tree_get(tree, 53));
        fprintf(stderr, "key:63 data:%d\n", bplus_tree_get(tree, 63));
        fprintf(stderr, "key:90 data:%d\n", bplus_tree_get(tree, 90));
        fprintf(stderr, "key:88 data:%d\n", bplus_tree_get(tree, 88));
        fprintf(stderr, "key:15 data:%d\n", bplus_tree_get(tree, 15));
        fprintf(stderr, "key:10 data:%d\n", bplus_tree_get(tree, 10));
        fprintf(stderr, "key:44 data:%d\n", bplus_tree_get(tree, 44));
        fprintf(stderr, "key:68 data:%d\n", bplus_tree_get(tree, 68));

        /* Not found */
        fprintf(stderr, "key:100 data:%d\n", bplus_tree_get(tree, 100));

        /* Clear all */
        fprintf(stderr, "\n> Clear all...\n");
        for (i = 1; i <= 100; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Not found */
        fprintf(stderr, "key:100 data:%d\n", bplus_tree_get(tree, 100));
}

static void bplus_tree_insert_delete_test(struct bplus_tree *tree)
{
        int i, max_key = 100;

        fprintf(stderr, "\n> B+tree insertion and deletion testing...\n");

        /* Ordered insertion and deletion */
        fprintf(stderr, "\n-- Insert 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\n-- Delete 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Ordered insertion and reversed deletion */
        fprintf(stderr, "\n-- Insert 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\n-- Delete %d to 1, dump:\n", max_key);
        while (--i > 0) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Reversed insertion and ordered deletion */
        fprintf(stderr, "\n-- Insert %d to 1, dump:\n", max_key);
        for (i = max_key; i > 0; i--) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\n-- Delete 1 to %d, dump:\n", max_key);
        for (i = 1; i <= max_key; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Reversed insertion and reversed deletion */
        fprintf(stderr, "\n-- Insert %d to 1, dump:\n", max_key);
        for (i = max_key; i > 0; i--) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\n-- Delete %d to 1, dump:\n", max_key);
        for (i = max_key; i > 0; i--) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);
}

static void bplus_tree_normal_test(void)
{
        struct bplus_tree *tree;
        struct bplus_tree_config config;

        fprintf(stderr, "\n>>> B+tree normal test.\n");

        /* Init b+tree */
        config.order = 7;
        config.entries = 10;
        tree = bplus_tree_init(config.order, config.entries);
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
        bplus_tree_normal_test();
        //bplus_tree_abnormal_test();
        return 0;
}
