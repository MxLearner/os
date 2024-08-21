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
    if (size == 0 || size > MAX_KALLOC_SIZE)
    {
        return NULL;
    }
    if (size < 64)
    {
        size = 64;
    }
    else
    {
        size = (size + 63) & ~63;
    }
    lock(&kernel_lock);
    free_node *current = head;
    void *ret = NULL;
    while (current != NULL)
    {
        void *ptr = (void *)current + sizeof(free_node);
        ptr = (void *)(((uintptr_t)ptr + size - 1) & ~(size - 1));
        size_t total_size = (uintptr_t)ptr + size - (uintptr_t)current;
        if (total_size <= current->size)
        {
            if (total_size + sizeof(free_node) <= current->size)
            {
                // 足够空间插入一个新的空闲节点
                free_node *new_free_node = (free_node *)((uintptr_t)ptr + size);
                new_free_node->size = current->size - total_size;
                new_free_node->next = current->next;
                new_free_node->prev = current;
                if (current->next)
                {
                    current->next->prev = new_free_node;
                }
                current->next = new_free_node;
            }
            current->size = (uintptr_t)ptr - (uintptr_t)current - sizeof(free_node);
            ret = ptr;
            break;
        }
        current = current->next;
    }
    return ret;
}

static void kfree(void *ptr)
{
    // TODO
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