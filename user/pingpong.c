#include "kernel/types.h"
#include "user/user.h"


int
main(int argc, char *argv[]) 
{
  int p0[2];
  int p1[2];
  char buf;

  if (pipe(p0) < 0 || pipe(p1) < 0) {
      fprintf(2, "pipe: system call failed\n");
      exit(1);
  }

  int pid = fork();

  if (pid < 0) {
    fprintf(2, "fork: system call failed\n");
    exit(1);
  }

  if (pid == 0) {
    int read_fd = dup(p0[0]);
    int write_fd = dup(p1[1]);
    close(p0[0]);
    close(p0[1]);
    close(p1[0]);
    close(p1[1]);

    if (read(read_fd, &buf, 1) < 0) {
      fprintf(2, "read: system call failed\n");
      exit(1);
    }
    fprintf(1, "%d: received ping\n", getpid());

    if (write(write_fd, "x", 1) != 1) {
      fprintf(2, "write: system call failed\n");
      exit(1);
    }

    close(read_fd);
    close(write_fd);

    exit(0);
  } else {
    int write_fd = dup(p0[1]);
    int read_fd = dup(p1[0]);
    close(p0[0]);
    close(p0[1]);
    close(p1[0]);
    close(p1[1]);

    if (write(write_fd, "x", 1) != 1) {
      fprintf(2, "write: system call failed\n");
      exit(1);
    }

    if (read(read_fd, &buf, 1) < 0) {
      fprintf(2, "read: system call failed\n");
      exit(1);
    }

    fprintf(1, "%d: received pong\n", getpid());


    close(write_fd);
    close(read_fd);
  }

  exit(0);
}
