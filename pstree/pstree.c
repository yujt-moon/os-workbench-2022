//
// Created by Jiangtao Yu on 2024/6/9.
// https://jyywiki.cn/OS/2022/labs/M1.html
// TODO 待改进点：1.某些进程消失后会产生错误，进程树打印的不够优美
//
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#define FALSE 0
#define TRUE 1
#define PROC_PATH "/proc"
#define MAX_PROC_NAME 128
#define MAX_CHILDREN_COUNT 128

// 定义 bool 类型
typedef unsigned int bool;

/**
 * 进程信息
 */
struct proc_info
{
    int pid;                    // 进程 id
    char name[MAX_PROC_NAME];   // 进程名称
    int ppid;                   // 进程的父 id
};

/**
 * 节点信息
 */
struct tree_node
{
    int pid;
    struct proc_info *info;                             // 当前进程信息
    struct tree_node *children[MAX_CHILDREN_COUNT];     // 子进程信息
};

void getFlags(int, char**, bool*, bool*, bool*);
void getProcTree(void);
void buildTree(struct tree_node*, struct proc_info*);
void printTree(struct tree_node*, int);
char *constructName(struct proc_info *);
void sortTree(struct tree_node*);
void swap(struct tree_node**, struct tree_node**);
void quickSort(struct tree_node**, int, int);

bool isShowPids = FALSE;
bool isNumbericSort = FALSE;
char *printName;

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        // printf("argv[%d] = %s\n", i , argv[i]);
    }
    assert(!argv[argc]);

    printName = malloc(MAX_PROC_NAME * sizeof(char));

    bool showVersion = FALSE;

    getFlags(argc, argv, &isShowPids, &isNumbericSort, &showVersion);

    if (showVersion) {
        printf("pstree v1.0.0 "
               "Copyright (C) 2024 yujt-moon. \n"
        );
        return 0;
    }

    getProcTree();

    return 0;
}

/**
 * 获取进程树
 */
void getProcTree() {
    // 获取所有进程号
    DIR *dir = NULL;
    struct dirent *entry;
    FILE *fp;
    char path[512];
    char str[BUFSIZ];

    if ((dir = opendir(PROC_PATH)) == NULL) {
        printf("readdir path [%s] failed!\n", PROC_PATH);
        exit(1);
    }

    struct tree_node *root = (struct tree_node *) malloc(sizeof(struct tree_node));

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name)) {
            // printf("dir name is %s\n", entry->d_name);
            // 获取当前文件夹下的 /proc/[pid]/stat
            sprintf(path, "%s/%s/stat", PROC_PATH, entry->d_name);
            fp = fopen(path, "r");
            if (fp == NULL) {
                printf("read stat file failed, path is [%s].\n", path);
                exit(1);
            }
            struct proc_info *info = (struct proc_info *) malloc(sizeof(struct proc_info));
            info->pid = atoi(entry->d_name);
            while (fgets(str, BUFSIZ - 1, fp)) {
                // printf("stat file content: %s", str);
                char *wp = strtok(str, " ");
                int index = 0;
                while ((wp = strtok(NULL, " "))) {
                    index++;
                    if (index == 1) {
                        // 只拷贝左括号之后的，并将右括号设置为 '\0'
                        // 可能存在的 ((xxx)) 形式的结果
                        char *p = strrchr(wp, ')');
                        *p = '\0';
                        strcpy(info->name, ++wp);
                    }
                    if (index == 3) {
                        info->ppid = atoi(wp);
                        break;
                    }
                }
            }
            buildTree(root, info);

            // printf("proc_info: pid=[%6d], ppid=[%6d], name=[%s]\n", info->pid, info->ppid, info->name);
            fclose(fp);
        }
    }

    if (isNumbericSort)
        sortTree(root);
    printTree(root, 0);
}

/**
 * 构造树结构
*/
void buildTree(struct tree_node *root, struct proc_info *info) {
    if (root->info == NULL) {
        root->pid = info->pid;
        root->info = info;
    } else if (root->pid == info->ppid) {
        for (int i = 0; i < MAX_PROC_NAME; i++) {
            if (root->children[i] == NULL) {
                root->children[i] = (struct tree_node *) malloc(sizeof(struct tree_node));
                root->children[i]->pid = info->pid;
                root->children[i]->info = info;
                break;
            }
        }
    } else {
        for (int i = 0; i < MAX_PROC_NAME; i++) {
            if (root->children[i] != NULL) {
                buildTree(root->children[i], info);
            }
        }
    }
}

/**
 * 递归排序，快速排序可以使用C库函数提供的快速排序
 *
 * @param root
 */
void sortTree(struct tree_node *root) {
    int i;
    for (i = 0; root->children[i] != NULL; i++)
        ;
    quickSort(root->children, 0, i - 1);
    for (int j = 0; j < i; j++) {
        sortTree(root->children[j]);
    }
}

int partition(struct tree_node *nodes[], int low, int high) {
    struct tree_node *pivot = nodes[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (strcasecmp(nodes[j]->info->name, pivot->info->name) <= 0) {
            i++;
            swap(&nodes[i], &nodes[j]);
        }
    }
    swap(&nodes[i+1], &nodes[high]);
    return i + 1;
}

/**
 * 快速排序（可以使用库函数）
 *
 * @param nodes
 * @param low
 * @param high
 */
void quickSort(struct tree_node *nodes[], int low, int high) {
    if (low < high) {
        int pivot = partition(nodes, low, high);

        quickSort(nodes, low, pivot - 1);
        quickSort(nodes, pivot + 1, high);
    }
}

/**
 * 交换
 *
 * @param a
 * @param b
 */
void swap(struct tree_node **a, struct tree_node **b) {
    struct tree_node *tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

/**
 * 打印进程树，实现存在问题
 *
 * @param root
 * @param count
 */
void printTree(struct tree_node *root, int count) {
    if (root->children[0] == NULL)
        printf("\n");
    if (count == 0)
        printf("%s", constructName(root->info));

    for (int i = 0; root->children[i] != NULL; i++) {
        if (i == 0)
            printf("\u2500\u252c\u2500%s", constructName(root->children[i]->info));
        if (i != 0) {
            // 打印前导空格
            printf("%*s", (int) strlen(constructName(root->info)) + count, "");
            if (root->children[i+1] != NULL) {
                printf(" \u251c\u2500%s", constructName(root->children[i]->info));
            } else {
                printf(" \u2514\u2500%s", constructName(root->children[i]->info));
            }
        }
        // printf(i == 0 ? "%s" : (root->children[i+1] != NULL && (root + sizeof(struct tree_node *)) == NULL) ? " \u251c\u2500%s" : " \u2514\u2500%s", constructName(root->children[i]->info));
        printTree(root->children[i], (int) strlen(constructName(root->info)) + count + 3);
    }
}

/**
 * 构造进程名称（支持可选 pid）
 *
 * @param info
 * @return
 */
char *constructName(struct proc_info *info) {
    if (!isShowPids)
        return info->name;
    // sprintf(printName, "%s(%-5d)", info->name, info->pid);
    sprintf(printName, "%s(%d)", info->name, info->pid);
    return printName;
}

/**
 * 获取命令行中的参数，使用 getopt 库
 *
 * @param argc
 * @param argv
 * @param isShowPids 是否显示 pid
 * @param isNumbericSort 是否排序
 * @param showVersion 显示版本信息
 */
void getFlags(int argc, char **argv, bool *isShowPids, bool *isNumbericSort, bool *showVersion) {
    // 获取所有参数
    char *optstring = "pnV";
    static struct option long_options[] = {
            {"show-pids",       no_argument,    0, 0},
            {"numberic-sort",   no_argument,    0, 0},
            {"version",         no_argument,    0, 0},
            {0,                 0,              0, 0}
    };

    int c;
    while (TRUE) {
        int option_index = 0;
        c = getopt_long(argc, argv, optstring, long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                if (option_index == 0)
                    *isShowPids = TRUE;
                else if (option_index == 1)
                    *isNumbericSort = TRUE;
                else if (option_index == 2)
                    *showVersion = TRUE;
                break;
            case 'p':
                *isShowPids = TRUE;
                break;
            case 'n':
                *isNumbericSort = TRUE;
                break;
            case 'V':
                *showVersion = TRUE;
                break;
            case '?':
                break;
            default:
                printf("?? getopt returned character code 0%o ??\n", c);
        }
    }
}