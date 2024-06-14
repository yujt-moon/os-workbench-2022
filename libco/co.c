#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>
#include <time.h>

enum co_status {
    CO_NEW = 1,     // 新创建，还未执行过
    CO_RUNNING,     // 已经执行过
    CO_WAITING,     // 在 co_wait 上等待
    CO_DEAD,        // 已经结束，但还未释放资源
};

struct co {
    const char *name;           // 协程名
    void (*func)(void *);       // co_start 指定的入口地址和参数
    void *arg;
    enum co_status status;      // 协程的状态
    struct co *waiter;          // 是否有其他协程在等待当前协程
    jmp_buf context;            // 寄存器现场（setjmp.h）
    uint8_t stack[STACK_SIZE];  // 协程的堆栈
};

struct co *sched();
void release_waiting(struct co *current);

static struct co *co_routines[CO_COUNT];

static struct co *current;

void __attribute__((constructor)) before_main() {
    // 设置随机数种子
    srand((unsigned) time(NULL));
    // 将 main 函数封装成一个协程
    struct co *co_main = (struct co *) malloc(sizeof(struct co));
    co_main->name = "main";
    co_main->status = CO_RUNNING;
    co_routines[0] = co_main;

    // 当前协程设置为 main 方法
    current = co_main;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // 初始化协程
    struct co *co_routine = (struct co *) malloc(sizeof(struct co));
    co_routine->name = name;
    co_routine->func = func;
    co_routine->arg = arg;
    co_routine->status = CO_NEW;

    // 将协程加入到树组中
    int i = 0;
    while (co_routines[i] != NULL) {
        i++;
        if (i >= CO_COUNT) {
            exit(1);
        }
    }
    co_routines[i] = co_routine;
    return co_routine;
}

void co_wait(struct co *co) {
    co->waiter = current;
    current->status = CO_WAITING;
    co_yield();
}

void co_yield() {
    // 判断调度的协程是不是自己，如果是直接返回，否则需要保存当前的协程的上下文
//    debug("enter %s\n", __FUNCTION__);
    struct co *tmp = sched();
    if (tmp == current)
        return;

    int val = setjmp(current->context);

    // 调用之后的返回结果
    if (val == 0) {
        // 当前执行的进程被挂起，选择可以运行的程序运行
        current = tmp;
        if (current->status == CO_NEW) {
            current->status == CO_RUNNING;
            current->func(current->arg);
            current->status = CO_DEAD;
            // 释放等待在当前协程的程序，使其成为 running 状态
            release_waiting(current);
            // 执行结束
            debug("[%s] execute end!\n", current->name);
        }
    } else {    // longjmp 后的返回结果
        longjmp(current->context, 1);
    }
}

// 协程调度
struct co *sched() {
    int count = 0;
    int *idxs = calloc((size_t) CO_COUNT, sizeof(int));
    for (int i = 0; (i < CO_COUNT) && (co_routines[i] != NULL); ++i) {
        if (co_routines[i]->status == CO_NEW || co_routines[i]->status == CO_RUNNING) {
            debug("[%s] can be sched ...\n", co_routines[i]->name);
            idxs[count] = i;
            count++;
        }
    }

    int idx = rand() % count;
//    debug("current sched is [%s] ...\n", co_routines[idxs[idx]]->name);
    debug("%s|%s:", co_routines[idxs[idx]]->name, (char *)co_routines[idxs[idx]]->arg);
    return co_routines[idxs[idx]];
}

void release_waiting(struct co *current) {
    for (int i = 0; (i < CO_COUNT) && (co_routines[i] != NULL); ++i) {
        if (co_routines[i]->waiter == current) {
            co_routines[i]->waiter = NULL;
            co_routines[i]->status = CO_RUNNING;
        }
    }
    if (current->status != CO_RUNNING)
        co_yield();
}
