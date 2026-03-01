#include "kernel/types.h"
#include "user/user.h"

void
sieve(int read_fd)
{
  int p[2];
  int prime, buf;

  if (read(read_fd, &prime, sizeof(int)) == sizeof(int)) {
    fprintf(1, "prime %d\n", prime);
  } else {
    exit(0);
  }

  if (pipe(p) < 0) {
    fprintf(2, "pipe: system call failed\n");
    exit(1);
  }

  if (fork() == 0) {
    close(p[1]);

    sieve(p[0]);
  } else {
    int write_fd = dup(p[1]);
    close(p[0]);
    close(p[1]);

    while (read(read_fd, &buf, sizeof(int)) == sizeof(int)) {
      if (buf % prime == 0) {
        continue;
      }

      if (write(write_fd, &buf, sizeof(int)) != sizeof(int)) {
        fprintf(2, "primes: write error\n");
        exit(1);
      }
    }

    close(write_fd);
    wait(0);
  }

  exit(0);
}

int
main(int argc, char *argv[]) 
{
  int p[2];
  if (pipe(p) < 0) {
    fprintf(2, "pipe: system call failed\n");
    exit(1);
  }

  if (fork() == 0) {
    int read_fd = dup(p[0]);
    close(p[0]);
    close(p[1]);

    sieve(read_fd);
  } else {
    int write_fd = dup(p[1]);
    close(p[0]);
    close(p[1]);

    for (int i = 2; i <= 35; i++) {
      if (write(write_fd, &i, sizeof(int)) != sizeof(int)) {
        fprintf(2, "primes: write error\n");
        exit(1);
      }
    }

    close(write_fd);

    wait(0);
  }

  exit(0);
}
