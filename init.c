// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf(1, "init: starting sh\n");
    printf(1, "Modified by: Parsa Ghorbani & Arman Rostami\n");
    pid = fork();
    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    char* name = " _____________________________________\n\
< By Arman Rostami and Parsa Ghorbani > \n\
 -------------------------------------\n\
        \\   ^__^\n\
         \\  (oo)\\_______\n\
            (__)\\       )\\/\\ \n\
                ||----w |\n\
                ||     ||\n";
    printf(1, name);
    
    while((wpid=wait()) >= 0 && wpid != pid)
      printf(1, "zombie!\n");
  }
}
