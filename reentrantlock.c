// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "reentrantlock.h"

void
initreentrantlock(struct reentrantlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
  lk->owner_pid = 1000;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquire_reentrantlock(struct reentrantlock *lk)
{
  int current_pid = 0;
  pushcli(); // disable interrupts to avoid deadlock.
  struct proc* myp = myproc();
  if (myp)
    current_pid =  myp->pid;

  if(lk->owner_pid != current_pid && holding_reentrantlock(lk))
    panic("acquire");

  // The xchg is atomic.
  while(xchg(&lk->locked, 1) != 0 && lk->owner_pid != current_pid)
    ;

  if (current_pid != 0 && lk->owner_pid == current_pid)
    popcli();

  if (current_pid != 0)
    lk->owner_pid = current_pid;


  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();

  // Record info about lock acquisition for debugging.
  lk->cpu = mycpu();
  getcallerpcs(&lk, lk->pcs);
}

// Release the lock.
void
release_reentrantlock(struct reentrantlock *lk)
{
  if(!holding_reentrantlock(lk))
    panic("release");

  lk->pcs[0] = 0;
  lk->cpu = 0;
  lk->owner_pid = 1000;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m" (lk->locked) : );

  popcli();
}

// Check whether this cpu is holding the lock.
int
holding_reentrantlock(struct reentrantlock *lock)
{
  int r;
  pushcli();
  r = lock->locked && lock->cpu == mycpu();
  popcli();
  return r;
}