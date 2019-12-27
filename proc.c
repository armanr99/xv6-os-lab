#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "barrier.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


struct {
  struct spinlock lock;
  struct barrierlock barrierlocks[NBARRIERLOCK];
} btable;

static struct proc *initproc;

int nextpid = 1;
int createdProcessesCount = 0;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&btable.lock, "btable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  
  p->lottery_ticket = 50;
  p->schedule_queue = LOTTERY;

  p->arrival_time = ticks + createdProcessesCount++;
  p->executed_cycle_number = 1;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    p = schedule_lottery();

    if (p == 0)
      p = schedule_hrrn();
    if (p == 0)
      p = schedule_srpf();

    if (p != 0) {
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      c->proc->executed_cycle_number++;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();
  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int
get_parent_id(void)
{
  struct proc *curproc = myproc();
  return curproc->parent->pid;
}

int
get_children(int pid, char* buf, int buf_size)
{
  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->parent->pid == pid)
    {
      int endIndex = 0;
      while(endIndex < buf_size && buf[endIndex] != '\0')
        endIndex++;

      if(endIndex > buf_size - 2)
        exit();
      
      buf[endIndex] = (p->pid + '0');
      buf[endIndex + 1] = '\0';
    }
  }
  release(&ptable.lock);
  return 0;
}

int
get_posteriors(int pid, char* buf, int buf_size)
{
  struct proc *p;

  int children[100];
  int children_idx = 0;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->parent->pid == pid)
    {
      int endIndex = 0;
      while(endIndex < buf_size && buf[endIndex] != '\0')
        endIndex++;

      if(endIndex > buf_size - 2)
        exit();
      
      buf[endIndex] = (p->pid + '0');
      buf[endIndex + 1] = '\0';

      children[children_idx++] = p->pid;
    }
  }

  for (int i = 0; i < children_idx; i++)
    get_posteriors(children[i], buf, buf_size);
  
  release(&ptable.lock);
  return 0;
}

int 
set_sleep(int n)
{

  uint ticks0;
  ticks0 = ticks;

  while(ticks - ticks0 < n * 100)
      sti();
  
  return 0;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int get_random_number(int bound)
{
  int random;
  int ticksMod = (ticks % bound);
  random = (ticksMod * ticksMod) % bound;
  random = (random * ticksMod) % bound;
  random = (random + (1000000007) % bound) % bound;
  return random;
}

struct proc*
schedule_lottery(void) 
{
  struct proc *p;

  int sum_lotteries = 1;
  int random_ticket = 0;
  struct proc *highLottery_ticket = 0;
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) 
  {
    if(p->state == RUNNABLE && p->schedule_queue == LOTTERY)
      sum_lotteries += p->lottery_ticket;
  }
  if (sum_lotteries == 1)
    return 0;

  random_ticket = get_random_number(sum_lotteries);
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state == RUNNABLE && p->schedule_queue == LOTTERY)
    {
      random_ticket -= p->lottery_ticket;
      if(random_ticket <= 0) {
        highLottery_ticket = p;
        break;
      }
    }
  }
  return highLottery_ticket;
}


struct proc*
schedule_hrrn(void)
{
  struct proc *p;
  struct proc *maxP = 0;
  double maxHRRN = -1;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) 
  {
    if(p->state == RUNNABLE && p->schedule_queue == HRRN)
    {
      int waitingTime = ticks - p->arrival_time;
      double hrrn = (double) waitingTime / (double) p->executed_cycle_number;
      if(hrrn > maxHRRN) 
      {
        maxP = p;
        maxHRRN = hrrn;
      }
    }
  }
  
  return maxP;
}

struct proc*
schedule_srpf(void)
{
  struct proc *p;
  struct proc *selectedP = 0;
  double min_remaining_priority = 100000;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == RUNNABLE && p->schedule_queue == SRPF && p->remaining_priority < min_remaining_priority)
    {
      min_remaining_priority = p->remaining_priority;
      selectedP = p;
    }
  if (selectedP != 0 && selectedP->remaining_priority >= 0.1)
    selectedP->remaining_priority -= 0.1;
  return selectedP;
}

void
set_lottery_ticket(int lottery_ticket, int pid)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->pid == pid)
    {
      p->lottery_ticket = lottery_ticket;
      break;
    }
}

void
set_srpf_remaining_priority(int remaining_priority, int num_of_decimal, int pid)
{
  double actual_remaining_priority = remaining_priority;
  while(num_of_decimal--)
    actual_remaining_priority /= 10.0;
  
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->pid == pid)
    {
      p->remaining_priority = actual_remaining_priority;
      break;
    }
}

void
set_schedule_queue(int schedule_queue, int pid)
{
  struct proc *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->pid == pid)
    {
      p->schedule_queue = schedule_queue;
      break;
    }
}

int 
int_size(int n)
{
  if(n == 0)
    return 1;
  
  int ret = 0;
  while(n > 0)
  {
    ret++;
    n /= 10;
  }
  return ret;
}

char* 
get_state_name(int state){
  static char *states[] = {
  [UNUSED]    "UNUSED  ",
  [EMBRYO]    "EMBRYO  ",
  [SLEEPING]  "SLEEPING",
  [RUNNABLE]  "RUNNABLE",
  [RUNNING]   "RUNNING ",
  [ZOMBIE]    "ZOMBIE  "
  };
  return states[state];
}

char*
get_queue_name(int schedule_queue)
{
  switch(schedule_queue)
  {
    case LOTTERY:
      return "LOTTERY";
    case HRRN:
      return "HRRN   ";
    case SRPF:
      return "SRPF   ";
    default:
      return "UNKNOWN";
  }
}


void
ps()
{
  struct proc *p;
  int name_spaces = 0;
  int i = 0 ;
  char* state;
  char* queue_name;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == 0)
      continue;
    if( name_spaces < strlen(p->name))
      name_spaces = strlen(p->name);
  }

  cprintf("name");
  for(i = 0; i < name_spaces - strlen("name") + 3 ; i++)
    cprintf(" ");
  
  cprintf("pid");
  for(i = 0; i < 4; i++)
    cprintf(" ");
  cprintf("state");
  for(i = 0; i < 6; i++)
    cprintf(" ");
  cprintf("queue");
  for(i = 0 ; i < 5; i++)
    cprintf(" ");
  cprintf("remaining_priority");
  for(i = 0; i < 3; i++)
    cprintf(" ");
  cprintf("lottery_tickets");
  for(i = 0; i < 3; i++)
    cprintf(" ");
  cprintf("execution_cycles");
  for(i = 0; i < 3; i++)
    cprintf(" ");
  cprintf("HRRN");
  for(i = 0; i < 7; i++)
    cprintf(" ");
  cprintf("createTime");
  for(i = 0; i < 3; i++)
    cprintf(" ");
  cprintf("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == 0)
      continue;
    
    cprintf("%s", p->name);
    for(i = 0; i < name_spaces - strlen(p->name) + 3 ; i++)
      cprintf(" ");
    
    cprintf("%d", p->pid);
    for(i = 0; i < 7 - int_size(p->pid); i++)
      cprintf(" ");
    
    state = get_state_name(p->state);
    cprintf("%s" , state);
    for(i = 0; i < 11 - strlen(state); i++)
      cprintf(" ");
    
    queue_name =  get_queue_name(p->schedule_queue);
    cprintf("%s ", queue_name);
    for(i = 0; i < 9 - strlen(queue_name); i++)
      cprintf(" ");
    // cprintf("%d", (int)(p->remaining_priority * 10));
    cprintf("%d.%d ", (int)(p->remaining_priority * 10) / 10, (int)(p->remaining_priority * 10) % 10);
    for(i = 0; i < 20 - int_size((int)(p->remaining_priority * 10)) - 2; i++)
      cprintf(" ");
    
    cprintf("%d  ", p->lottery_ticket);
    for(i = 0; i < 16 - int_size(p->lottery_ticket); i++)
      cprintf(" ");

    cprintf("%d ", p->executed_cycle_number);
    for (i = 0; i < 18 - int_size(p->executed_cycle_number); i++)
      cprintf(" ");

    cprintf("%d/%d ", (ticks + createdProcessesCount - p->arrival_time), p->executed_cycle_number);
    for (i = 0; i < 10 - int_size(ticks + createdProcessesCount - p->arrival_time) - int_size(p->executed_cycle_number) - 1; i++)
      cprintf(" ");
    
    cprintf("%d  ", p->arrival_time);
    cprintf("\n");
  }
}


// Phase 4
int
initbarrierlock(int max_processes_count)
{
  int bid = 0;

  acquire(&btable.lock);
  
  for(int i = 0; i < NBARRIERLOCK; i++)
  {
    if (btable.barrierlocks[i].locked == 0)
    {
      bid = i;
      initbarrier(&btable.barrierlocks[i], max_processes_count);
      break;
    }
  }

  release(&btable.lock);
  return bid;
}

void
acquirebarrierlock(int bid)
{
  acquire(&btable.lock);
  struct barrierlock *b = 0;
  for (int i = 0; i < NBARRIERLOCK; i++)
    if(bid == i)
    {
      b = &btable.barrierlocks[i];
      break;
    }
  release(&btable.lock);
  acquirebarrier(b);
}
