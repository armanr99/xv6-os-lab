#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "date.h"

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
    printf(1, "0. test count num of digits\n");
    printf(1, "1. test get parent id\n");
    printf(1, "2. test get childern\n");
    printf(1, "3. test get posteriors\n");
    printf(1, "4. test sleep\n");

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
      char ans[100];
      strcpy(ans, "");

      int child1_pid = fork();
      if (child1_pid != 0){
        printf(1, "Current process id:");
        itoaprint(getpid());
        get_children(1, ans, 100);
        printf(1, "children of 1: %s\n", ans);
        wait();
      }
    }
    else if (atoi(buf) == 3)
    {
      char ans[100];
      strcpy(ans, "");

      int child1_pid = fork();
      if (child1_pid != 0){
        printf(1, "Current process id:");
        itoaprint(getpid());
        get_posteriors(1, ans, 100);
        printf(1, "posteriors of 1: %s\n", ans);
        wait();
      }
    }
    else if (atoi(buf) == 4)
    {
      printf(1, "Enter sleep time: ");
      read(0, buf, strlen(buf));

      struct rtcdate beginDate, endDate;
      fill_date(&beginDate);
      set_sleep(atoi(buf));
      fill_date(&endDate);

      printf(1, "Entered sleep time: %s", buf);
      printf(1, "Calculated sleep time: ");
      itoaprint(endDate.second - beginDate.second);
    }
    exit();
}