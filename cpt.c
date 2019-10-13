#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define LINE_MODE 1 
#define BUF_SIZE 512

char buf[BUF_SIZE];

void transferFd(int inFd, int outFd, int mode)
{
  int n;

  while((n = read(inFd, buf, sizeof(buf))) > 0) 
  {
    if(write(outFd, buf, n) != n)
    {
      printf(1, "cpt: write error\n");
      exit();
    }

    if(n < 0)
    {
      printf(1, "cpt: read error\n");
      exit();
    }

    if(mode == LINE_MODE && buf[n - 1] == '\n')
      return;
  }
}

void cptOne(char* outFile){
  unlink(outFile);
  
  int outFd = open(outFile, O_CREATE | O_WRONLY);

  if(outFd < 0)
  {
    printf(1, "cpt: error in opening or creating output file\n");
    exit();
  }

  transferFd(0, outFd, LINE_MODE);
  close(outFd);
}

void cptTwo(char* inFile, char* outFile){
  unlink(outFile);

  int inFd = open(inFile, 0);
  int outFd = open(outFile, O_CREATE | O_WRONLY);

  if(inFd < 0)
  {
    printf(1, "cpt: error in opening input file\n");
    exit();
  }
  else if(outFd < 0)
  {
    printf(1, "cpt: error in opening or creating output file\n");
    exit();
  }

  transferFd(inFd, outFd, 0);
  close(inFd);
  close(outFd);
}

int main(int argc, char *argv[]){
  if(argc <= 1)
  {
    printf(1, "cpt: too few arguments\n");
    exit();
  }
  else if(argc == 2)
  {
    cptOne(argv[1]);
    exit();
  }
  else if(argc == 3)
  {
    cptTwo(argv[1], argv[2]);
    exit();
  }
  else
  {
    printf(1, "cpt: too many arguments\n");
    exit();
  }
}
