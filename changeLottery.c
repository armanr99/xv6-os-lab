#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "date.h"

int main(int argc, char *argv[]) 
{
    if(argc != 3)
    {
        printf(2, "Too few or too much arguments.\n");
        exit();
    }
    int pid = atoi(argv[1]);
    int lottery_ticket = atoi(argv[2]);
    set_lottery_ticket(lottery_ticket, pid);
    exit();
}