#ifndef XV6_SYNC_H
#define XV6_SYNC_H

#define BUFFER_SIZE 10

struct buffer {
  int data[BUFFER_SIZE];
  int count;
  int in;
  int out;
};

void buffer_init(void);
void buffer_produce(int item);
int buffer_consume(void);

extern struct spinlock buffer_lock;
extern struct spinlock produce_lock;
extern struct spinlock consume_lock;
extern struct proc *producer_proc;
extern struct proc *consumer_proc;

#endif
