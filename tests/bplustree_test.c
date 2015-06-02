#include <stdio.h>
#include <stdlib.h>

#include "bplustree.h"

static void
stdin_flush(void)
{
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
                continue;
        }
}

int
main(void)
{
        int i, ret, again;
        int level, order, entries, max_key;
        struct bplus_tree *tree;

        do {
                fprintf(stdout, "Set b+tree max level (<= 10 e.g. 5): ");
                if ((i = getchar()) == '\n') {
                        level = 5;
                } else {
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%d", &level);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else if (level > MAX_LEVEL) {
                                again = 1;
                        } else {
                                again = 0;
                        }
                }
        } while (again);

        do {
                fprintf(stdout, "Set b+tree max order (<= 1024 e.g. 7): ");
                if ((i = getchar()) == '\n') {
                        order = 7;
                } else {
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%d", &order);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else if (order > MAX_ORDER) {
                                again = 1;
                        } else {
                                again = 0;
                        }
                }
        } while (again);

        do {
                fprintf(stdout, "Set b+tree max entries (<= 4096 e.g. 10): ");
                if ((i = getchar()) == '\n') {
                        entries = 10;
                } else {
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%d", &entries);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else if (entries > MAX_ENTRIES) {
                                again = 1;
                        } else {
                                again = 0;
                        }
                }
        } while (again);

        do {
                fprintf(stdout, "Set b+tree max key (e.g. 100): ");
                if ((i = getchar()) == '\n') {
                        max_key = 100;
                } else {
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%d", &max_key);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else {
                                again = 0;
                        }
                }
        } while (again);

        /* Init b+tree */
        tree = bplus_tree_init(level, order, entries);
        if (tree != NULL) {
                fprintf(stderr, "b+tree CRUD testing...\n");
        } else {
                fprintf(stderr, "Init failure!\n");
                exit(-1);
        }

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

        /* Deinit b+tree */
        bplus_tree_deinit(tree);

        return 0;
}
