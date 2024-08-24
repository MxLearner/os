#include <common.h>

static void test_pmm()
{
    for (int i = 0; i < 10; i++)
    {
        size_t size = 12 + i;
        void *ptr = pmm->alloc(size);
        printf("kalloc_ptr: %p\n", ptr);
        // pmm->free(ptr);
    }
}

static void os_init()
{
    pmm->init();
}

static void os_run()
{
    for (const char *s = "Hello World from CPU #*\n"; *s; s++)
    {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }
    test_pmm();
    while (1)
        ;
}

MODULE_DEF(os) = {
    .init = os_init,
    .run = os_run,
};
