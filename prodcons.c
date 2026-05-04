#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_ITEMS 20

void producer(void);
void consumer(void);

int
main(int argc, char *argv[])
{
  int pid;

  printf(1, "=== Producer-Consumer Model Test ===\n");
  printf(1, "Buffer size: 10, Items to produce: %d\n\n", NUM_ITEMS);

  pid = fork();
  if(pid == 0) {
    producer();
    exit();
  } else {
    consumer();
    wait();
    printf(1, "\n=== Test Complete ===\n");
    exit();
  }
}

void
producer(void)
{
  int i;
  printf(1, "[PRODUCER] Started (pid=%d)\n", getpid());

  for(i = 1; i <= NUM_ITEMS; i++) {
    printf(1, "[PRODUCER] Calling produce(%d)\n", i);
    produce(i);
    sleep(1);
  }
  printf(1, "[PRODUCER] Finished producing %d items\n", NUM_ITEMS);
}

void
consumer(void)
{
  int i, item;
  printf(1, "[CONSUMER] Started (pid=%d)\n", getpid());

  for(i = 0; i < NUM_ITEMS; i++) {
    printf(1, "[CONSUMER] Calling consume()\n");
    item = consume();
    printf(1, "[CONSUMER] Received: %d\n", item);
    sleep(1);
  }
  printf(1, "[CONSUMER] Finished consuming %d items\n", NUM_ITEMS);
}
