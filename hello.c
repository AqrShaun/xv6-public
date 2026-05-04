#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "Calling hello() system call...\n");
  hello();
  printf(1, "hello() system call returned\n");
  exit();
}
