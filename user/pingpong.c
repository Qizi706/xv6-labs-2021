#include "kernel/types.h"
#include "user/user.h"


int
main(int argc, char *argv[]) 
{
  int p1[2];
  int p2[2];
  char buf;
  if (pipe(p1) < 0 || pipe(p2) < 0) {
      fprintf(2, "pipe() failed\n");
      exit(1);
  }

  int pid = fork();

  if (pid < 0) {
    fprintf(2, "fork: system call failed\n");
    exit(1);
  }

  if (pid == 0) {
    close(p1[1]);
    close(p2[0]);

    if (read(p1[0], &buf, 1) < 0) {
      fprintf(2, "read: system call failed\n");
      exit(1);
    }
    fprintf(2, "%d: received ping\n", getpid());

    if (write(p2[1], "x", 1) != 1) {
      fprintf(2, "write: system call failed\n");
      exit(1);
    }

    close(p1[0]);
    close(p2[1]);
    exit(0);
  } else {
    if (write(p1[1], "x", 1) != 1) {
      fprintf(2, "write: system call failed\n");
      exit(1);
    }
    if (read(p2[0], &buf, 1) < 0) {
      fprintf(2, "read: system call failed\n");
      exit(1);
    }

    fprintf(2, "%d: received pong\n", getpid());
  } 
  exit(0);
}
