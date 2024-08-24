#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define STACK_SIZE 4 * 1024 * 8
const int CO_NUM = 128;

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg)
{
    asm volatile(
#if __x86_64__
        "movq %%rsp,-0x10(%0); leaq -0x20(%0), %%rsp; movq %2, %%rdi ; call *%1; movq -0x10(%0) ,%%rsp;"
        :
        : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
        : "memory"
#else
        "movl %%esp, -0x8(%0); leal -0xC(%0), %%esp; movl %2, -0xC(%0); call *%1;movl -0x8(%0), %%esp"
        :
        : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
        : "memory"
#endif
    );
}
// "movq %%rsp,-0x10(%0);"      // 保存当前栈指针 (`rsp`) 到新栈的栈顶位置减去0x10处。
//     "leaq -0x20(%0), %%rsp;" // 更新栈指针 (`rsp`) 为新栈顶 (`sp`) 减去0x20（16字节对齐）。
//     "movq %2, %%rdi ;"       // 将参数 `arg` 传递给寄存器 `rdi`，这是x86_64架构下第一个函数参数的标准寄存器。
//     "call *%1;"              // 调用目标函数 `entry`。
//     "movq -0x10(%0) ,%%rsp;" // 函数返回后，恢复原来的栈指针 (`rsp`)。

enum co_status
{
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD,    // 已经结束，但还未释放资源
};

struct co
{
    char name[256];
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;
    struct co *next;               // 用于链接到调度器的队列中
    enum co_status status;         // 协程的状态
    struct co *waiter;             // 是否有其他协程在等待当前协程
    jmp_buf context;               // 寄存器现场
    uint8_t stack[STACK_SIZE + 1]; // 协程的堆栈
};
struct co *current;

void co_main()
{
    current = (struct co *)malloc(sizeof(struct co));
    current->status = CO_RUNNING;
    current->waiter = NULL;
    strcpy(current->name, "main");
    current->next = current;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg)
{
    struct co *start = (struct co *)malloc(sizeof(struct co));
    start->arg = arg;
    start->func = func;
    start->status = CO_NEW;
    strcpy(start->name, name);
    if (current == NULL) // init main
    {
        co_main();
    }

    struct co *t = current;
    while (t->next != current)
    {
        t = t->next;
    }
    t->next = start;
    start->next = current;
    return start;
}

void co_wait(struct co *co)
{
    current->status = CO_WAITING;
    co->waiter = current;
    while (co->status != CO_DEAD)
    {
        co_yield ();
    }
    current->status = CO_RUNNING;
    struct co *t = current;
    while (t->next != co)
    {
        t = t->next;
    }
    t->next = t->next->next;
    free(co);
}

void co_yield ()
{
    if (current == NULL) // init main
    {
        co_main();
    }
    assert(current);
    int val = setjmp(current->context);
    if (val == 0)
    {
        struct co *co_next = current;
        do
        {
            co_next = co_next->next;
        } while (co_next->status == CO_DEAD || co_next->status == CO_WAITING);
        current = co_next;
        if (current->status == CO_NEW)
        {
            current->status = CO_RUNNING;
            stack_switch_call(&current->stack[STACK_SIZE], (void *)current->func, (uintptr_t)current->arg);
            current->status = CO_DEAD;
            if (current->waiter != NULL)
            {
                current = current->waiter;
            }
        }
        else
        {
            longjmp(current->context, 1);
        }
    }
    else
    {
        return;
    }
}
