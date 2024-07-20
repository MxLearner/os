// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>

// int main()
// {
//     os->init();
//     mpe_init(os->run);

//     return 1;
// }

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
        // struct malloc_op op = random_op();

        // switch (op.type)
        // {
        // case OP_ALLOC:
        // {
        //     printf("cpu_current:%s,alloc(%d)\n", cpu_current(), op.sz);
        //     // void *ptr = pmm->alloc(op.sz);
        //     //  alloc_check(ptr, op.sz);
        //     break;
        // }
        // case OP_FREE:
        //     printf("cpu_current:%s,free(%p)\n", cpu_current(), op.addr);
        //     // free(op.addr);
        //     break;
        // }
        printf("Hello World from CPU #%d\n", cpu_current());
    }
}

int main()
{

    os->init();
    printf("cpu_count: %d\n", cpu_count());
    mpe_init(stress_test);
    return 0;
}