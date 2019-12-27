// Barrier locks

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "barrier.h"

void
initbarrier(struct barrierlock *lk, int max_processes_count)
{
    initlock(&lk->lk, "barrier lock");
    lk->locked = 1;
    lk->max_processes_count = max_processes_count;
}

void
acquirebarrier(struct barrierlock *lk)
{
    acquire(&lk->lk);
    lk->cur_processes_count++;
    if (lk->cur_processes_count == lk->max_processes_count)
        wakeup(lk);
    else
        sleep(lk, &lk->lk);
    release(&lk->lk);
}