#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

void itoaprint(int n) {
  char ret[100];
  if (n == 0)
    printf(1, "0");

  int idx = 0;
  while(n > 0){
    ret[idx++] = (n % 10) + '0';
    n /= 10;
  }

  for (int i = 0; i < idx / 2; i++)
  {
    char tmp = ret[i];
    ret[i] = ret[idx - i - 1];
    ret[idx - i - 1] = tmp;
  }
  
  ret[idx] = '\n';
  ret[idx+1] = '\0';
  printf(1, ret);
}

int main(int argc, char *argv[]) 
{
    printf(1, "What system call do you like to test ? \n");
    printf(1, "0. count_num_of_digitst\n");
    printf(1, "1. test get parent id\n");
    printf(1, "2. test get childern\n");
    printf(1, "3. test sleep\n");
    // printf(1, "0.enable/disable\n");
    // printf(1, "1.invoked_syscalls\n");
    // printf(1, "2.get_count\n");
    // printf(1 ,"3.sort_syscalls\n");
    // printf(1, "4.log_syscalls\n");
    // printf(1, "5.inc_num\n");
  

    char buf[1024];
    read(0, buf, 1024);
    if(atoi(buf) == 0)
    {
        printf(1, "enter number: \n");
        read(0, buf, 1024);
        int number = atoi(buf);
        int backup;
        __asm__("movl %%edx, %0" : "=r" (backup));
        __asm__("movl %0, %%edx" :  : "r"(number) );
        __asm__("movl $22 , %eax;");
        __asm__("int $64");
        __asm__("movl %0, %%edx" :  : "r"(backup) );
    }
    else if (atoi(buf) == 1)
    {
        int child_pid = fork();
        if (child_pid != 0){
            itoaprint(get_parent_id(child_pid));
            itoaprint(getpid());
        }
        wait();
    }
    else if (atoi(buf) == 2)
    {
      int child1_pid = fork();
      if (child1_pid != 0){
        printf(1, "current process id:");
        itoaprint(getpid());
        get_children(getpid());
        printf(1, "children of 1\n");
        get_children(1);
        wait();
      }
    }
    else if (atoi(buf) == 3)
    {
      set_sleep(3);
    }
    exit();
}