#ifndef EVENT_LOGGING_QUEUE_H
#define EVENT_LOGGING_QUEUE_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/err.h>

#include "buffer.h"

struct queue {
  struct list_head list;
  spinlock_t lock;
  unsigned long flags;
  wait_queue_head_t wait;
};

#define DEFINE_QUEUE(name) struct queue name = {	\
    .lock = __SPIN_LOCK_UNLOCKED(name.lock),		\
    .list = LIST_HEAD_INIT(name.list),			\
    .wait = __WAIT_QUEUE_HEAD_INITIALIZER(name.wait)	\
  }						 

static inline void queue_lock(struct queue* queue) {
  spin_lock_irqsave(&queue->lock, queue->flags);
}

static inline void queue_unlock(struct queue* queue) {
  spin_unlock_irqrestore(&queue->lock, queue->flags);
}

static inline int queue_empty(struct queue* queue) {
  int ret;
  queue_lock(queue);
  ret = list_empty(&queue->list);
  queue_unlock(queue);
  return ret;
}

static inline void queue_put(struct queue* queue, struct sbuffer *buf) {
  queue_lock(queue);
  list_add_tail(&buf->list, &queue->list);
  queue_unlock(queue);
}

static inline struct sbuffer* __queue_peek_try(struct queue* queue) {
  if (!list_empty(&queue->list))
    return list_entry(queue->list.next, struct sbuffer, list);
  else
    return NULL;
}

static inline struct sbuffer* queue_take_try(struct queue* queue) {
  struct sbuffer* buf = NULL;
  queue_lock(queue);
  buf = __queue_peek_try(queue);
  if (buf != NULL)
    list_del_init(&buf->list);
  queue_unlock(queue);
  return buf;
}

static inline struct sbuffer* queue_take_interruptible(struct queue* queue) {
  struct sbuffer* ret = NULL;
  int err = 0;
  do {
    ret = queue_take_try(queue);
  } while ( NULL == ret &&
	    0 == (err = wait_event_interruptible(queue->wait, !queue_empty(queue))) );
  
  if (err)
    return ERR_PTR(err);
  else
    return ret;
}

static inline struct sbuffer* queue_peek_try(struct queue* queue) {
  struct sbuffer* buf = NULL;
  queue_lock(queue);
  buf = __queue_peek_try(queue);
  queue_unlock(queue);
  return buf;
}

static inline struct sbuffer* queue_peek_interruptible(struct queue* queue) {
  struct sbuffer* ret = NULL;
  int err = 0;
  do {
    ret = queue_peek_try(queue);
  } while ( NULL == ret && 
	    0 == (err = wait_event_interruptible(queue->wait, !queue_empty(queue))) );

  if (err)
    return ERR_PTR(err);
  else
    return ret;
}

#endif
