#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bplustree.h"

struct bplus_tree_config {
        char filename[1024];
        int block_size;
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
        int i, size, ret = 0, again = 1;

        printf("\n-- B+tree setting...\n");
        while (again) {
                printf("Set data index file name (e.g. /tmp/data.index): ");
                switch (i = getchar()) {
                case EOF:
                        printf("\n");
                case 'q':
                        return -1;
                case '\n':
                        strcpy(config->filename, "/tmp/data.index");
                        again = 0;
                        break;
                default:
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%s", config->filename);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else {
                                again = 0;
                        }
                        break;
                }
        }

        again = 1;
        while (again) {
                printf("Set index file block size (bytes, power of 2, e.g. 4096): ");
                switch (i = getchar()) {
                case EOF:
                        printf("\n");
                case 'q':
                        return -1;
                case '\n':
                        config->block_size = 4096;
                        again = 0;
                        break;
                default:
                        ungetc(i, stdin);
                        ret = fscanf(stdin, "%d", &size);
                        if (!ret || getchar() != '\n') {
                                stdin_flush();
                                again = 1;
                        } else if (size <= 0 || (size & (size - 1)) != 0) {
                		fprintf(stderr, "Block size must be positive and pow of 2!\n");
                                again = 1;
                        } else if (size <= 0 || (size & (size - 1)) != 0) {
                                again = 1;
                        } else {
                                int order = (size - sizeof(struct bplus_node)) / (sizeof(key_t) + sizeof(off_t));
                                if (size < (int) sizeof(struct bplus_node) || order <= 2) {
                                        fprintf(stderr, "block size is too small for one node!\n");
                                        again = 1;
                                } else {
                                        config->block_size = size;
                                        again = 0;
                                }
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
                        printf("key:%d data_index:%ld\n", n, bplus_tree_get(tree, n));
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

        printf("\n");
        return -1;
}

static void command_tips(void)
{
        printf("i: Insert key. e.g. i 1 4-7 9\n");
        printf("r: Remove key. e.g. r 1-100\n");
        printf("s: Search by key. e.g. s 41-60\n");
        printf("d: Dump the tree structure.\n");
        printf("q: quit.\n");
}

static void command_process(struct bplus_tree *tree)
{
        int c;
        printf("Please input command (Type 'h' for help): ");
        for (; ;) {
                switch (c = getchar()) {
                case EOF:
                        printf("\n");
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
                case '\n':
                        printf("Please input command (Type 'h' for help): ");
                default:
                        break;
                }
        }
}

int main(void)
{
        struct bplus_tree_config config;
        struct bplus_tree *tree = NULL;
        while (tree == NULL) {
                if (bplus_tree_setting(&config) < 0) {
                        return 0;
                }
                tree = bplus_tree_init(config.filename, config.block_size);
        }
        command_process(tree);
        bplus_tree_deinit(tree);

        return 0;
}
