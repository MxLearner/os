#include <common.h>

typedef struct free_node
{
    size_t size;
    struct free_node *next;
    struct free_node *prev;
} free_node;

free_node *head = NULL;

typedef int LOCK;
LOCK kernel_lock;

const size_t MAX_KALLOC_SIZE = 16 * 1024 * 1024;

const LOCK LOCKED = 1;
const LOCK UNLOCKED = 0;

void lock_init(LOCK *lock)
{
    *lock = UNLOCKED;
}

void lock(LOCK *lock)
{
    while (atomic_xchg(lock, LOCKED))
        ;
}
void unlock(LOCK *lock)
{
    atomic_xchg(lock, UNLOCKED);
}

static void *kalloc(size_t size)
{
    // if (size == 0)
    //     return NULL;
    // if (size > MAX_KALLOC_SIZE)
    //     return NULL;
    // if (size < 64)
    // {
    //     size = 64;
    // }
    // else
    // {
    //     size = (size + 63) & ~63;
    // }
    // lock(&kernel_lock);
    // free_node *current = head;
    // free_node *new_free_node = NULL;
    // while (current != NULL)
    // {
    //     if (current->size >= size)
    //     {
    //         void *ptr = (void *)current + sizeof(free_node);
    //         if ((size_t)ptr % size != 0)
    //         {
    //             ptr = ptr + (size - (size_t)ptr % size);
    //         }
    //     }
    // }
    return NULL;
}

static void kfree(void *ptr)
{
    // TODO
    // You can add more .c files to the repo.
}

static void pmm_init()
{
    uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);

    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end);

    head = (free_node *)heap.start;
    head->size = pmsize;
    head->next = NULL;
    head->prev = NULL;
    lock_init(&kernel_lock);
}

MODULE_DEF(pmm) = {
    .init = pmm_init,
    .alloc = kalloc,
    .free = kfree,
};