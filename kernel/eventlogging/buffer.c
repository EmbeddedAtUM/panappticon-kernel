#include <linux/bootmem.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/gfp.h>

#include "buffer.h"

// max order is likely 10 or 11
int sbuffer_init(struct sbuffer* buf, unsigned int order) {
  void* addr;

  addr = (void*) __get_free_pages(GFP_ATOMIC, order);
  if (!addr)
    goto err;

  INIT_LIST_HEAD(&buf->list);
  buf->order = order;
  buf->start = addr;
  buf->end   = addr + (1 << order) * PAGE_SIZE;
  buf->rp    = addr;
  buf->wp    = addr;
  return 0;

 err:
  return -ENOMEM;
}

void sbuffer_free(struct sbuffer* buf) {
  free_pages((unsigned long) buf->start, buf->order);
}

void sbuffer_clear(struct sbuffer* buf) {
  buf->rp = buf->start;
  buf->wp = buf->start;
}

/* Returns NULL if too full */
void* sbuffer_reserve(struct sbuffer* buf, int len) {
  void* old_wp = buf->wp;
  if (buf->wp + len <= buf-> end) {
    buf->wp += len;
    return old_wp;
  } else {
    return NULL;
  }
}

/* Returns the specified number of bytes to the buffer */
void sbuffer_cancel(struct sbuffer* buf, int len) {
  buf->wp -= len;
}

int sbuffer_empty(struct sbuffer* buf) {
  return (buf->rp == buf->wp);
}

int sbuffer_read(struct sbuffer* buf, char* page, int count) {
  int len, num;
  int avail;
  
  len = 0;
  avail = buf->wp - buf->rp;
  num = min(count, avail);

  if (num > 0) {
    memcpy(page, buf->rp, num);
    buf->rp += num;
    len += num;
  }

  return len;
}


