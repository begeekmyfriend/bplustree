#include <stdio.h>
#include <stdlib.h>

#include "bplustree.h"

struct bplus_tree_config {
        int order;
        int entries;
}; 

static void stdin_flush(void)
{
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
                continue;
        }
}

static int bplus_tree_setting(struct bplus_tree_config *config)
{
        int i, ret = 0, again = 1;

        fprintf(stderr, "\n-- B+tree setting...\n");
        fprintf(stderr, "Set b+tree non-leaf order (%d < order <= %d e.g. 7): ", BPLUS_MIN_ORDER, BPLUS_MAX_ORDER);
        while (again) {
                switch (i = getchar()) {
                case EOF:
                        fprintf(stderr, "\n");
                        return -1;
                case 'q':
                        return -1;
                case '\n':
                        config->order = 7;
                        again = 0;
                        break;
                default:
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%d", &config->order);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else if (config->order < BPLUS_MIN_ORDER || config->order > BPLUS_MAX_ORDER) {
                                again = 1;
                        } else {
                                again = 0;
                        }
                        break;
                }
        }

        again = 1;
        fprintf(stderr, "Set b+tree leaf entries (<= %d e.g. 10): ", BPLUS_MAX_ENTRIES);
        while (again) {
                switch (i = getchar()) {
                case EOF:
                        fprintf(stderr, "\n");
                        return -1;
                case 'q':
                        return -1;
                case '\n':
                        config->entries = 10;
                        again = 0;
                        break;
                default:
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%d", &config->entries);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else if (config->entries > BPLUS_MAX_ENTRIES) {
                                again = 1;
                        } else {
                                again = 0;
                        }
                        break;
                }
        }

        return ret;
}

static void _proc(struct bplus_tree *tree, char op, int n)
{
        switch (op) {
                case 'i':
                        bplus_tree_put(tree, n, n);
                        break;
                case 'r':
                        bplus_tree_put(tree, n, 0);
                        break;
                case 's':
                        fprintf(stderr, "key:%d data:%d\n", n, bplus_tree_get(tree, n));
                        break;
                default:
                        break;
        }       
}

static int number_process(struct bplus_tree *tree, char op)
{
        int c, n = 0;
        int start = 0, end = 0;

        while ((c = getchar()) != EOF) {
                if (c == ' ' || c == '\t' || c == '\n') {
                        if (start != 0) {
                                if (n >= 0) {
                                        end = n;
                                } else {
                                        n = 0;
                                }
                        }

                        if (start != 0 && end != 0) {
                                if (start <= end) {
                                        for (n = start; n <= end; n++) {
                                                _proc(tree, op, n);
                                        }
                                } else {
                                        for (n = start; n >= end; n--) {
                                                _proc(tree, op, n);
                                        }
                                }
                        } else {
                                if (n != 0) {
                                        _proc(tree, op, n);
                                }
                        }

                        n = 0;
                        start = 0;
                        end = 0;

                        if (c == '\n') {
                                return 0;
                        } else {
                                continue;
                        }
                }

                if (c >= '0' && c <= '9') {
                        n = n * 10 + c - '0';
                } else if (c == '-' && n != 0) {
                        start = n;
                        n = 0;
                } else {
                        n = 0;
                        start = 0;
                        end = 0;
                        while ((c = getchar()) != ' ' && c != '\t' && c != '\n') {
                                continue;
                        }
                        ungetc(c, stdin);
                }
        }

        fprintf(stderr, "\n");
        return -1;
}

static void command_tips(void)
{
        fprintf(stderr, "i: Insert key. e.g. i 1 4-7 9\n");
        fprintf(stderr, "r: Remove key. e.g. r 1-100\n");
        fprintf(stderr, "s: Search by key. e.g. s 41-60\n");
        fprintf(stderr, "d: Dump the tree structure.\n");
        fprintf(stderr, "q: quit.\n");
}

static void command_process(struct bplus_tree *tree)
{
        int c;
        fprintf(stderr, "Please input command (Type 'h' for help): ");
        for (; ;) {
                switch (c = getchar()) {
                case EOF:
                        fprintf(stderr, "\n");
                case 'q':
                        return;
                case 'h':
                        command_tips();
                        break;
                case 'd':
                        bplus_tree_dump(tree);
                        break;
                case 'i':
                case 'r':
                case 's':
                        if (number_process(tree, c) < 0) {
                                return;
                        }
                        break;
                case '\n':
                        fprintf(stderr, "Please input command (Type 'h' for help): ");
                        break;
                default:
                        break;
                }
        }
}

int main(void)
{
        struct bplus_tree *tree;
        struct bplus_tree_config config;

        /* B+tree default setting */
        if (bplus_tree_setting(&config) < 0) {
                return 0;
        }

        /* Init b+tree */
        tree = bplus_tree_init(config.order, config.entries);
        if (tree == NULL) {
                fprintf(stderr, "Init failure!\n");
                exit(-1);
        }

        /* Operation process */
        command_process(tree);

        /* Deinit b+tree */
        bplus_tree_deinit(tree);

        return 0;
}
