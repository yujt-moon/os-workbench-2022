#include <stdio.h>

struct co* co_start(const char *name, void (*func)(void *), void *arg);
void co_yield();
void co_wait(struct co *co);

#define STACK_SIZE 32000
#define CO_COUNT 128

#ifdef LOCAL_MACHINE
#define debug(...) printf(__VA__ARGS__);
#else
#define debug(...)
#endif

typedef unsigned char uint8_t;
