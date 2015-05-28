#include <stdio.h>
#include <stdlib.h>

#include "bplustree.h"

int
main(void)
{
        int level = 5;
        int order = 7;
        int entries = 10;
        struct bplus_tree *tree;

        tree = bplus_tree_init(level, order, entries);
        if (tree != NULL) {
                fprintf(stderr, "B+tree CRUD testing...\n");
        } else {
                fprintf(stderr, "Init failure!\n");
                exit(-1);
        }
#if 0
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
#endif

#if 0
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
#endif

#if 0
        fprintf(stderr, "key:24 data:%d\n", bplus_tree_get(tree, 24));
        fprintf(stderr, "key:72 data:%d\n", bplus_tree_get(tree, 72));
        fprintf(stderr, "key:01 data:%d\n", bplus_tree_get(tree, 1));
        fprintf(stderr, "key:39 data:%d\n", bplus_tree_get(tree, 39));
        fprintf(stderr, "key:53 data:%d\n", bplus_tree_get(tree, 53));
        fprintf(stderr, "key:63 data:%d\n", bplus_tree_get(tree, 63));
        fprintf(stderr, "key:90 data:%d\n", bplus_tree_get(tree, 90));
        fprintf(stderr, "key:88 data:%d\n", bplus_tree_get(tree, 88));
        fprintf(stderr, "key:15 data:%d\n", bplus_tree_get(tree, 15));
        fprintf(stderr, "key:10 data:%d\n", bplus_tree_get(tree, 10));
        fprintf(stderr, "key:44 data:%d\n", bplus_tree_get(tree, 44));
        fprintf(stderr, "key:68 data:%d\n", bplus_tree_get(tree, 68));
        fprintf(stderr, "key:74 data:%d\n", bplus_tree_get(tree, 74));
        fprintf(stderr, "key:44 data:%d\n", bplus_tree_get(tree, 44));
        fprintf(stderr, "key:45 data:%d\n", bplus_tree_get(tree, 45));
        fprintf(stderr, "key:46 data:%d\n", bplus_tree_get(tree, 46));
        fprintf(stderr, "key:47 data:%d\n", bplus_tree_get(tree, 47));
        fprintf(stderr, "key:48 data:%d\n", bplus_tree_get(tree, 48));
#endif

#if 0
        bplus_tree_put(tree, 90, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 88, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 74, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 72, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 68, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 63, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 53, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 44, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 39, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 24, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 15, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 10, 0);
        bplus_tree_dump(tree);
        bplus_tree_put(tree, 1, 0);
        bplus_tree_dump(tree);
#endif

#if 1
#define MAX_KEY  100

        int i;

        /* Ordered insertion and deletion */
        fprintf(stderr, "\nInsert 1 to 100, dump:\n");
        for (i = 1; i <= MAX_KEY; i++) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\nDelete 1 to 100, dump:\n");
        for (i = 1; i <= MAX_KEY; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Ordered insertion and reversed deletion */
        fprintf(stderr, "\nInsert 1 to 100, dump:\n");
        for (i = 1; i <= MAX_KEY; i++) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\nDelete 100 to 1, dump:\n");
        while (--i > 0) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Reversed insertion and ordered deletion */
        fprintf(stderr, "\nInsert 100 to 1, dump:\n");
        for (i = MAX_KEY; i > 0; i--) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\nDelete 1 to 100, dump:\n");
        for (i = 1; i <= MAX_KEY; i++) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);

        /* Reversed insertion and reversed deletion */
        fprintf(stderr, "\nInsert 100 to 1, dump:\n");
        for (i = MAX_KEY; i > 0; i--) {
                bplus_tree_put(tree, i, i);
        }
        bplus_tree_dump(tree);

        fprintf(stderr, "\nDelete 100 to 1, dump:\n");
        for (i = MAX_KEY; i > 0; i--) {
                bplus_tree_put(tree, i, 0);
        }
        bplus_tree_dump(tree);
#endif

        bplus_tree_deinit(tree);

        return 0;
}
