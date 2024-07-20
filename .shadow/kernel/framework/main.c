// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>

int main()
{
    os->init();
    mpe_init(os->run);

    return 1;
}

// #include <common.h>
// #include <klib.h>

// enum ops
// {
//     OP_ALLOC = 1,
//     OP_FREE
// };

// struct malloc_op
// {
//     enum ops type;
//     union
//     {
//         size_t sz;  // OP_ALLOC: size
//         void *addr; // OP_FREE: address
//     };
// };

// struct malloc_op * random_op()
// {
//     struct malloc_op op;
//     if (rand() % 2)
//     {
//         op.type = OP_ALLOC;
//         op.sz = rand() % 1024;
//     }
//     else
//     {
//         op.type = OP_FREE;
//         op.addr = (void *)rand();
//     }
//     return op;
// }

// void stress_test()
// {
//     while (1)
//     {
//         // 根据 workload 生成操作
//         struct malloc_op op = random_op();

//         switch (op.type)
//         {
//         case OP_ALLOC:
//         {
//             void *ptr = pmm->alloc(op.sz);
//             alloc_check(ptr, op.sz);
//             break;
//         }
//         case OP_FREE:
//             free(op.addr);
//             break;
//         }
//     }
// }

// int main()
// {
//     os->init();
//     mpe_init(stress_test);
//     return 0;
// }