#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc < 2) {
	printf("too fewer argument\n");
	exit(0);
  }
  int time = atoi(argv[1]);
  sleep(time);

  exit(0);
}
