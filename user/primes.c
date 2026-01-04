#include "kernel/types.h"
#include "user/user.h"

void
sieve(int pleft[2]) 
{
  int pright[2];
  int prime;
  int n;

  close(pleft[1]); // close write end of pleft

  if (read(pleft[0], &prime, sizeof(int)) == 0) {
    close(pleft[0]);
    exit(0);
  }

  fprintf(2, "prime %d\n", prime);
  pipe(pright);

  if (fork() == 0) {
    sieve(pright);
  } else {
    close(pright[0]);

    while (read(pleft[0], &n, sizeof(int)) != 0) {
      if (n % prime != 0) {
        write(pright[1], &n, sizeof(int));
      }
    }
  }

  close(pleft[0]);
  close(pright[1]);
  wait(0);
  exit(0);
}

int
main(int argc, char *argv[]) 
{
  int p[2];
  pipe(p);

  if (fork() == 0) {
    sieve(p);
  } else {
    close(p[0]);
    for (int i = 2; i <= 35; i++) {
      write(p[1], &i, sizeof(int));
    }
    close(p[1]);

    wait(0);
  }
  exit(0);
}
