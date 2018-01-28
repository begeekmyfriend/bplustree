#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "bplustree.h"

static unsigned int valid_ins;
static unsigned int valid_query;
static unsigned int valid_del;

static void show_running_info(void)
{
        printf("$$ VALID INSERT:%u\n",valid_ins);
        printf("$$ VALID QUERY:%u\n",valid_query);
        printf("$$ VALID DELETE:%u\n",valid_del);
        printf("###DONE###\n");
}

static unsigned char huge_array[INT_MAX>>3] = { 0 };

#define TEST_KEY 0
#define has(a, k)       ((a[(k)>>3]) & (1<<((k)&7)))
#define set(a, k)       ((a[(k)>>3]) |= (1<<((k)&7)))
#define unset(a, k)     ((a[(k)>>3]) &= ~(1<<((k)&7)))
//#define DEBUG
#ifdef DEBUG
#define log(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define log(...)
#endif

void exec_file(char *file, struct bplus_tree *tree)
{
        int got = 0, exist = 0;
        int k;
        int ret;
        char op;
        FILE *fp = fopen(file,"r");
        if (!fp) {
                fprintf(stderr, "Where is the testing data?\n");
                exit(-1);
        }

        for (; ;) {
                ret = fscanf(fp," %c %d", &op, &k);
                if (ret == EOF) {
                        fclose(fp);
                        return;
                }

#ifdef DEBUG
                if (got) {
                        if (-1 == bplus_tree_get(tree, TEST_KEY)) {
                                //bplus_tree_dump(tree);
                                return;
                        }
                }
#endif

                switch(op)
                {
                case 'i':
                        log("i %d ", k);
                        if (has(huge_array, k)) {
                                exist = 1;
                        } else {
                                exist = 0;
                                set(huge_array, k);
                        }
                        if (0 == bplus_tree_put(tree, k, k)) {
                                valid_ins++;
                                if (k == TEST_KEY) {
                                        got = 1;
                                }
                                if (exist) {
                                        fprintf(stderr, "insert key %d error, has been inserted before!\n", k);
                                        exit(-1);
                                }
                        } else {
                                if (!exist) {
                                        fprintf(stderr, "insert key %d error, could be inserted!\n", k);
                                        exit(-1);
                                }
                        }
                        break;
                case 's':
                        log("s %d ", k);
                        if (has(huge_array, k)) {
                                exist = 1;
                        } else {
                                exist = 0;
                        }
                        ret = bplus_tree_get(tree, k);
                        if (ret != -1) {
                                valid_query++;
                                if (!exist) {
                                        fprintf(stderr, "search key %d error, found but not exists!\n", k);
                                        exit(-1);
                                }
                                assert(ret == k);
                        }
                        else {
                                if (exist) {
                                        fprintf(stderr, "search key %d error, exists but not found!\n", k);
                                        exit(-1);
                                }
                        }
                        break;
                case 'r':
                        log("d %d ", k);
                        if (has(huge_array, k)) {
                                exist = 1;
                                unset(huge_array, k);
                        } else {
                                exist = 0;
                        }
                        if (0 == bplus_tree_put(tree, k, 0))
                        {
                                valid_del++;
                                if (k == TEST_KEY && got) {
                                        got = 0;
                                }
                                if (!exist) {
                                        fprintf(stderr, "delete key %d error, deleted but not found!\n", k);
                                        exit(-1);
                                }
                        } else {
                                if (exist) {
                                        fprintf(stderr, "delete key %d error, found but not deleted!\n", k);
                                        exit(-1);
                                }
                        }
                        break;
                case 'q':
                        fclose(fp);
                        return;
                }
                log("\n");
        }
}

int main(void)
{
        struct bplus_tree *tree = bplus_tree_init("/tmp/coverage.index", 512);
        exec_file("testcase", tree);
        show_running_info();
        /* test range search */
        bplus_tree_get_range(tree, 10000, 100000);
        bplus_tree_get_range(tree, 100000, 10000);
        bplus_tree_deinit(tree);

        tree = bplus_tree_init("/tmp/coverage.index", 128);
        bplus_tree_deinit(tree);

        return 0;
}
