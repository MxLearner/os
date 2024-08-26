#include <os.h>
#include <common.h>
#include <limits.h>
#include <test.h>
static Context *kmt_context_save(Event, Context *);
static Context *kmt_schedule(Event, Context *);
spinlock_t kt;
spinlock_t tr;
extern task_t *alltasks[MAX_TASKS];
extern size_t task_cnt;
extern task_t *currents[MAX_CPU]; // 每个CPU的current线程
typedef struct CPU
{
    int intena; // 中断信息
    int noff;   // 递归深度
} CPU;
CPU cpus[MAX_CPU];
static void push_off()
{
    int i = ienabled();
    iset(false);
    int c = cpu_current();
    if (cpus[c].noff == 0)
        cpus[c].intena = i;
    cpus[c].noff++;
}
static void pop_off()
{
    int c = cpu_current();
    assert(cpus[c].noff >= 1);
    cpus[c].noff--;
    if (cpus[c].noff == 0 && cpus[c].intena == true)
    {
        iset(true);
    }
}
static void spin_init(spinlock_t *lk, const char *name)
{
    lk->lock = 0;
    lk->cpu = -1;
    strcpy(lk->name, name);
}
static void spin_lock(spinlock_t *lk)
{
    while (atomic_xchg(&lk->lock, 1) != 0)
    {
        if (ienabled())
            yield();
    }
    // for (volatile int i = 0; i < 10000; ++i)
    //     ;
    push_off(); // disable interrupts to avoid deadlock.
    lk->cpu = cpu_current();
}
static void spin_unlock(spinlock_t *lk)
{
    assert(lk->cpu == cpu_current());
    atomic_xchg(&lk->lock, 0);
    lk->cpu = -1;
    pop_off();
}

static Context *kmt_context_save(Event ev, Context *context)
{
    _current->context = *context;
    _current->status = RUNABLE;
    return NULL;
}
static Context *kmt_schedule(Event ev, Context *context)
{
    size_t i;
    kmt->spin_lock(&kt);
    if (task_cnt == _current->index)
        i = 0;
    else
        i = _current->index + 1;
    int num = 0;
    for (; i <= task_cnt; i++)
    {
        ++num;
        if (alltasks[i] && alltasks[i]->status != BLOCKED && alltasks[i]->status != RUNNING)
            break;
        if (i == task_cnt)
        {
            i = 0;
        }
        assert(num < 100);
    }
    kmt->spin_unlock(&kt);
#ifdef DEBUG
    printf("\nCPUID:%d , from [%s] schedule to [%s]\n", cpu_current(), _current->name, alltasks[i]->name);
#endif
    _current = alltasks[i];
    _current->status = RUNNING;
    return &(_current->context);
}
static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg)
{
    assert(task);
    task->status = RUNABLE;
    // printf("creat-thread\n");
    kmt->spin_lock(&kt);
    strcpy(task->name, name);
    Area stack = (Area){task->stack, task + 1};
    task->context = *kcontext(stack, entry, arg);
    size_t i = 0;
    for (; i < MAX_TASKS; i++)
    {
        if (alltasks[i] == NULL)
        {
            break;
        }
    }
    // printf("task->index: %d\n", i);
    alltasks[i] = task; // 加入线程池中
    task->index = i;
    if (i > task_cnt)
        task_cnt = i;
    kmt->spin_unlock(&kt);
    return 0;
}
static void kmt_teardown(task_t *task) // 清除任务
{
    kmt->spin_lock(&kt);
    alltasks[task->index] = NULL;
    if (task_cnt == task->index)
        task_cnt--;
    pmm->free(task);
    kmt->spin_unlock(&kt);
}
static void kmt_init()
{
    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save); // 注册了中断处理函数,最先
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);     // 中断中最后被调用
    for (size_t i = 0; i < 16; i++)
    {
        currents[i] = NULL;
        cpus[i].noff = 0;
        cpus[i].intena = 0;
    }
    for (size_t i = 0; i < MAX_TASKS; i++)
    {
        alltasks[i] = NULL;
    }
    task_cnt = 0;
    kmt->spin_init(&kt, "thread_lock_task"); // 保护任务相关的临界区
    kmt->spin_init(&tr, "trap_lock");        // 保护中断相关的临界区
}
static void sem_init(sem_t *sem, const char *name, int value)
{
    kmt->spin_init(&(sem->lock), name);
    sem->count = value;
    sem->l = 0;
    sem->r = 0;
    strcpy(sem->name, name);
}
void sem_wait_base(sem_t *sem)
{
    assert(sem);
    bool succ = false;
    while (!succ)
    {
        kmt->spin_lock(&(sem->lock));
        if (sem->count > 0)
        {
            sem->count--;
            succ = true;
        }
        kmt->spin_unlock(&(sem->lock));
        if (!succ)
        {
            if (ienabled()) // 如果中断开启
                yield();
        }
    }
}
void sem_signal_base(sem_t *sem)
{
    kmt->spin_lock(&(sem->lock));
    sem->count++;
    kmt->spin_unlock(&(sem->lock));
}
MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = spin_init,
    .spin_lock = spin_lock,
    .spin_unlock = spin_unlock,
    .sem_init = sem_init,
    .sem_signal = sem_signal_base,
    .sem_wait = sem_wait_base};
