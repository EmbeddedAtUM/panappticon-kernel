#include <linux/bootmem.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/gfp.h>

#include "buffer.h"

static DEFINE_SPINLOCK(empty_lock);
static LIST_HEAD(empty_list);

static DEFINE_SPINLOCK(full_lock);
static LIST_HEAD(full_list);

static DECLARE_WAIT_QUEUE_HEAD(full_wait);

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

int sbuffer_avail(struct sbuffer* buf) {
  return (buf->end - buf->wp);
}

int sbuffer_write(struct sbuffer* buf, char* page, int count) {
  if (sbuffer_avail(buf) < count)
    return 0;
  memcpy(buf->wp, page, count);
  buf->wp += count;
  return count;
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

void put_empty(struct sbuffer* buf) {
  unsigned long flags;
  spin_lock_irqsave(&empty_lock, flags);
  list_add(&buf->list, &empty_list);
  spin_unlock_irqrestore(&empty_lock, flags);
}

void put_full(struct sbuffer* buf) {
  unsigned long flags;
  spin_lock_irqsave(&full_lock, flags);
  list_add_tail(&buf->list, &full_list);
  spin_unlock_irqrestore(&full_lock, flags);
  //wake_up_interruptible(&full_wait);
}

struct sbuffer* take_empty_try(void) {
  struct sbuffer* ret = NULL;
  unsigned long flags;
  spin_lock_irqsave(&empty_lock, flags);
  if (0 == list_empty(&empty_list)) {
    ret = list_entry(empty_list.next, struct sbuffer, list);
    list_del_init(&ret->list);
  }
  spin_unlock_irqrestore(&empty_lock, flags);
  return ret;
}

struct sbuffer* __peek_full(void) {
  if (0 == list_empty(&full_list))
    return list_entry(full_list.next, struct sbuffer, list);
  else
    return NULL;
}

struct sbuffer* __take_full(void) {
  struct sbuffer* ret;
  ret = list_entry(full_list.next, struct sbuffer, list);
  list_del_init(&ret->list);
  return ret;
}

void recycle_if_empty(void) {
  struct sbuffer* buf;
  unsigned long flags;

  spin_lock_irqsave(&full_lock, flags);
  buf = __peek_full();
  if (NULL == buf)
    goto not_empty;
  if (0 == sbuffer_empty(buf))
    goto not_empty;
  buf = __take_full();
  spin_unlock_irqrestore(&full_lock, flags);
  
  sbuffer_clear(buf);
  put_empty(buf);
  return;

 not_empty:
  spin_unlock_irqrestore(&full_lock, flags);
}

int __full_list_empty(void) {
  int ret;
  unsigned long flags;
  spin_lock_irqsave(&full_lock, flags);
  ret = list_empty(&full_list);
  spin_unlock_irqrestore(&full_lock, flags);
  return ret;
}

/*
 * Returns, but does not remove, the head of the list of full buffers.
 * If the list is empty, block until a buffer is available.  Will
 * return NULL if the waiting process is interrupted while waiting.
 */
struct sbuffer* peek_full_blocking(void) {
  struct sbuffer* ret = NULL;
  unsigned long flags;
  do {
    spin_lock_irqsave(&full_lock, flags);
    ret = __peek_full();
    spin_unlock_irqrestore(&full_lock, flags);
  } while ( NULL == ret && 
	    0 == wait_event_interruptible(full_wait, 0 == __full_list_empty()) );
  return ret;
}


