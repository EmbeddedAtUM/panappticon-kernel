#include "buffer.h"
#include <linux/gfp.h>
#include <linux/bootmem.h>

int sbuffer_init(struct sbuffer* buf, int order) {
  void* addr;

  /* Use bootmem allocator, because get_free_pages is limited to less
     than 2^12 pages.  alloc_bootmem will panic if it fails, but we
     leave the error handling code in case we switch to a saner memory
     allocator.*/
  addr = alloc_bootmem((1 << order) * PAGE_SIZE);
  if (!addr)
    goto err;

  buf->order = order;
  buf->start = addr;
  buf->end   = addr + (1 << order) * PAGE_SIZE;
  buf->cur   = addr;
  
  return 0;

 err:
  return -ENOMEM;
}

void sbuffer_free(struct sbuffer* buf) {
  // Can't free bootmem and kernel will have paniced anyway.
  // Keep this method in case we migrate to a bette memory allocator
  // than bootmem.
}

int dbuffer_init(struct dbuffer* buf, int order) {
  int ret;

  ret = sbuffer_init(&buf->one, order);
  if (ret)
    goto err1;

  ret = sbuffer_init(&buf->two, order);
  if (ret)
    goto err2;

  spin_lock_init(&buf->lock);
  return 0;

 err2:
  sbuffer_free(&buf->one);
 err1:
  return ret;
}
