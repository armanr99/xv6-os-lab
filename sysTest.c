#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]) 
{
    printf(1, "What system call do you like to test? \n");
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
        __asm__("movl %0, %%edx" :  : "r"(number));
        __asm__("movl $22 , %eax;");
        __asm__("int $64");
        __asm__("movl %0, %%edx" :  : "r"(backup));
    }
    else if (atoi(buf) == 1)
    {
      if (fork() > 0)
      {
        printf(1, "Parent: pid: %d\n", getpid());
        wait();
      }
      else
        printf(1, "Child: parent pid: %d\n", get_parent_id());
    }
    else if (atoi(buf) == 2)
    {
      char ans[100];
      strcpy(ans, "");

      int child1_pid = fork();
      if (child1_pid != 0){
        printf(1, "Current process id: %d\n", getpid());
        get_children(getpid(), ans, 100);
        printf(1, "children of %d: %s\n", getpid(), ans);
        wait();
      }
    }
    else if (atoi(buf) == 3)
    {
      char ans[100];
      strcpy(ans, "");

      int child1_pid = fork();
      if (child1_pid != 0){
        printf(1, "Current process id: %d\n", getpid());
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

      int timeDiff = endDate.minute * 60 + endDate.second - beginDate.minute * 60 - beginDate.second;
      printf(1, "Entered sleep time: %s", buf);
      printf(1, "Calculated sleep time: %d\n", timeDiff);
    }
    exit();
}