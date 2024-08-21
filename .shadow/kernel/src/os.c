#include <common.h>

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
    void *ptr = pmm->alloc(100);
    printf("kalloc: %p\n", ptr);
    // pmm->free(ptr);
    // printf("kfree: %p\n", ptr);
    while (1)
        ;
}

MODULE_DEF(os) = {
    .init = os_init,
    .run = os_run,
};
