#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 3){
        printf(2, "wrong number of arguments\n");
    }
    else {
        char *new_path = argv[2];
        if (set_path(new_path) < 0)
            printf(2, "Can't read new path.\n");
        else
            printf(1, "New path set successfully.\n");
    }
    exit();
}