#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define BUF_MAX 10

void handleSingle(char* outFileName)
{
  // char* buf;

  // int i = 0; //TODO: cleanup
  // for(;;)
  // {
  //   char c;
  //   read(0, &c, 1);
  //   buf = (char*) realloc (char)
  //   buf[i] = c;
  // }
  // char buf[BUF_MAX];

  // read(0, buf, sizeof buf);

  // printf(1, "%s", buf);
}

void handleDouble(char* inFileName, char* outFileName)
{
  int inFile = open(inFileName, O_RDONLY);
  if(inFile < 0)
    printf(1, "Error: Input file was not found\n");

  // int outFile = open(outFileName, O_CREATE | O_WRONLY);
}

int main(int argc, char* argv[])
{
  if(argc == 2)
    handleSingle(argv[1]);
  else if(argc == 3)
    handleDouble(argv[1], argv[2]);
  else
    printf(1, "Error: Too many or few arguments detected\n");

  exit();
}
