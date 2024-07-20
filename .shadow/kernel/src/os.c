#include <common.h>

enum ops
{
    OP_ALLOC = 1,
    OP_FREE
};

struct malloc_op
{
    enum ops type;
    union
    {
        size_t sz;  // OP_ALLOC: size
        void *addr; // OP_FREE: address
    };
};

struct malloc_op random_op()
{
    struct malloc_op op;
    if (rand() % 2)
    {
        op.type = OP_ALLOC;
        op.sz = (rand() % 512 + 512) % 512;
    }
    else
    {
        op.type = OP_FREE;
        op.addr = (void *)1;
    }
    return op;
}

void stress_test()
{

    for (int i = 0; i < 2; i++)
    {
        struct malloc_op op = random_op();

        switch (op.type)
        {
        case OP_ALLOC:
        {
            printf("cpu_current:%d,alloc(%d)\n", cpu_current(), op.sz);
            // void *ptr = pmm->alloc(op.sz);
            //  alloc_check(ptr, op.sz);
            break;
        }
        case OP_FREE:
            printf("cpu_current:%d,free(%p)\n", cpu_current(), op.addr);
            // free(op.addr);
            break;
        }
    }
}

static void os_init()
{
    pmm->init();
}

static void os_run()
{
    // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    //     putch(*s == '*' ? '0' + cpu_current() : *s);
    // }
    printf("Hello World from CPU #%d\n", cpu_current());
    printf("cup_count: %d\n", cpu_count());
    stress_test();
    while (1)
        ;
}

MODULE_DEF(os) = {
    .init = os_init,
    .run = os_run,
};
