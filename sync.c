#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "sync.h"

struct buffer buf;

struct spinlock buffer_lock;
struct spinlock produce_lock;
struct spinlock consume_lock;
struct proc *producer_proc;
struct proc *consumer_proc;

void
buffer_init(void)
{
  initlock(&buffer_lock, "buffer");
  initlock(&produce_lock, "produce");
  initlock(&consume_lock, "consume");
  buf.count = 0;
  buf.in = 0;
  buf.out = 0;
  producer_proc = 0;
  consumer_proc = 0;
  cprintf("Buffer initialized: size=%d\n", BUFFER_SIZE);
}

void
buffer_produce(int item)
{
  acquire(&buffer_lock);

  while(buf.count >= BUFFER_SIZE) {
    sleep(&buf, &buffer_lock);
  }

  buf.data[buf.in] = item;
  buf.in = (buf.in + 1) % BUFFER_SIZE;
  buf.count++;

  cprintf("[PRODUCER] produced: %d, count=%d, buffer[%d]=%d\n",
          item, buf.count, (buf.in - 1 + BUFFER_SIZE) % BUFFER_SIZE, item);

  wakeup(&buf);
  release(&buffer_lock);
}

int
buffer_consume(void)
{
  int item;

  acquire(&buffer_lock);

  while(buf.count <= 0) {
    sleep(&buf, &buffer_lock);
  }

  item = buf.data[buf.out];
  buf.out = (buf.out + 1) % BUFFER_SIZE;
  buf.count--;

  cprintf("[CONSUMER] consumed: %d, count=%d\n", item, buf.count);

  wakeup(&buf);
  release(&buffer_lock);

  return item;
}
