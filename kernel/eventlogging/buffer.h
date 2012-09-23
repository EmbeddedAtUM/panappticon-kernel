#ifndef EVENT_LOGGING_BUFFER_H
#define EVENT_LOGGING_BUFFER_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>

struct sbuffer {
  int order;   // order of page allocation (2^order pages)
  void* start; // starting address
  void* end;   // last address in buffer
  void* cur;   // pointer to next free byte
};

struct dbuffer {
  struct sbuffer one;
  struct sbuffer two;
  spinlock_t lock;
};

int sbuffer_init(struct sbuffer* buf, int order);
void sbuffer_free(struct sbuffer* buf);

int dbuffer_init(struct dbuffer* buf, int order);

#endif
