#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_count_num_of_digits(void)
{
  struct proc *curproc = myproc();
  int num = curproc->tf->edx;
  int num_of_digits = 0;
  while(num > 0) num /= 10, num_of_digits++;
  cprintf("increased number is : %d \n", num_of_digits);
  return 1;
}

int
sys_set_path(void)
{
  char *new_path;
  if (argstr(0, &new_path) < 0)
    return -1;
  int new_path_len = strlen(new_path);
  int global_path_index = 0, new_dir_index = 0;
  char new_directory[1000];
  for (int i = 0; i < new_path_len; i++)
    if (new_path[i] != ':')
      new_directory[new_dir_index++] =  new_path[i];
    else{
      new_directory[new_dir_index] = '\0';
      for (int i = 0; i <= new_dir_index; i++)
        globalPath[global_path_index][i] = new_directory[i];
      new_dir_index = 0;
      global_path_index++;
    }
  len_global_path = global_path_index;
  return 1;
}

int
sys_get_parent_id(void)
{
  int pid = 0;
  if (argint(0, &pid) < 0)
    return -1;
  return get_parent_id(pid);
}

int
sys_get_children(void)
{
  int pid = 0;
  if (argint(0, &pid) < 0)
    return -1;
  get_children(pid);
  return 0;
}