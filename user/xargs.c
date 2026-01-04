#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
  char buf[512];
  char *x_argv[MAXARG];
  int x_argc = 0;

  if (argc < 2) {
    fprintf(2, "Usage: xargs command [args...]\n");
    exit(1);
  }

  for (int i = 1; i < argc; i++) {
    x_argv[x_argc] = argv[i];
    x_argc++;
  }

  int n = 0;
  char c;

  while (read(0, &c, 1) > 0) {
    if (c == '\n') {
      buf[n] = 0;

      if (fork() == 0) {
        x_argv[x_argc] = buf;
        x_argv[x_argc + 1] = 0;

        exec(x_argv[0], x_argv);
      } else {
        wait(0);
      }

      n = 0;
    } else {
      if (n < sizeof(buf) - 1) {
        buf[n++] = c;
      }
    }
  }

  exit(0);
}
