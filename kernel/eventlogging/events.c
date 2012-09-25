#include <linux/time.h>
#include <linux/sched.h>
#include <linux/smp.h>

#include <eventlogging/events.h>
#include "logging.h"

inline void event_log_header_init(struct event_hdr* event, u8 type) {
  struct timeval tv;
  do_gettimeofday(&tv);

  event->event_type = type;
  event->tv_sec = tv.tv_sec;
  event->tv_usec = tv.tv_usec;
  event->cpu = smp_processor_id();
  event->pid = current->pid;
}

static inline void event_log_simple(u8 event_type) {
  unsigned long flags;
  struct event_hdr event;
  local_irq_save(flags);
  event_log_header_init(&event, event_type);
  log_event(&event, sizeof(struct event_hdr));
  local_irq_restore(flags);
}

void event_log_context_switch(pid_t old, pid_t new) {
#ifdef CONFIG_EVENT_CONTEXT_SWITCH
  unsigned long flags;
  struct context_switch_event event;
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_CONTEXT_SWITCH);
  event.old_pid = old;
  event.new_pid = new;
  log_event(&event, sizeof(struct context_switch_event));
  local_irq_restore(flags);
#endif
}

void event_log_idle_start(void) {
#ifdef CONFIG_EVENT_IDLE_START
  event_log_simple(EVENT_IDLE_START);
#endif
}

void event_log_idle_end(void) {
#ifdef CONFIG_EVENT_IDLE_END
  event_log_simple(EVENT_IDLE_END);
#endif
}

void event_log_datagram_block(void) {
#ifdef CONFIG_EVENT_DATAGRAM_BLOCK
  event_log_simple(EVENT_DATAGRAM_BLOCK);
#endif
}

void event_log_datagram_resume(void) {
#ifdef CONFIG_EVENT_DATAGRAM_RESUME
  event_log_simple(EVENT_DATAGRAM_RESUME);
#endif
}

void event_log_stream_block(void) {
#ifdef CONFIG_EVENT_STREAM_BLOCK
  event_log_simple(EVENT_STREAM_BLOCK);
#endif
}

void event_log_stream_resume(void) {
#ifdef CONFIG_EVENT_STREAM_RESUME
  event_log_simple(EVENT_STREAM_RESUME);
#endif
}

void event_log_sock_block(void) {
#ifdef CONFIG_EVENT_SOCK_BLOCK
  event_log_simple(EVENT_SOCK_BLOCK);
#endif
}

void event_log_sock_resume(void) {
#ifdef CONFIG_EVENT_SOCK_RESUME
  event_log_simple(EVENT_SOCK_RESUME);
#endif
}

void event_log_io_block(void) {
#ifdef CONFIG_EVENT_IO_BLOCK
  event_log_simple(EVENT_IO_BLOCK);
#endif
}

void event_log_io_resume(void) {
#ifdef CONFIG_EVENT_IO_RESUME
  event_log_simple(EVENT_IO_RESUME);
#endif
}

void event_log_fork(pid_t pid, pid_t tgid) {
#ifdef CONFIG_EVENT_FORK
  unsigned long flags;
  struct fork_event event;
  
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_FORK);
  event.pid = pid;
  event.tgid = tgid;
  log_event(&event, sizeof(struct fork_event));
  local_irq_restore(flags);
#endif
}

void event_log_exit(void) {
#ifdef CONFIG_EVENT_EXIT
  event_log_simple(EVENT_EXIT);
#endif
}

void event_log_thread_name(struct task_struct* task) {
#ifdef CONFIG_EVENT_THREAD_NAME
   unsigned long flags;
   struct thread_name_event event;
  
   local_irq_save(flags);
   event_log_header_init(&event.hdr, EVENT_THREAD_NAME);
   memcpy(event.comm, task->comm, min(16, TASK_COMM_LEN));
   log_event(&event, sizeof(struct thread_name_event));
   local_irq_restore(flags);
#endif
}


void event_log_mutex_lock(void* lock) {
#ifdef CONFIG_EVENT_MUTEX_LOCK
  unsigned long flags;
  struct mutex_lock_event event;

  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_MUTEX_LOCK);
  event.lock = (__le32) lock;
  log_event(&event, sizeof(struct mutex_lock_event));
  local_irq_restore(flags);
#endif
}

void event_log_mutex_wait(void* lock) {
#ifdef CONFIG_EVENT_MUTEX_WAIT
  unsigned long flags;
  struct mutex_wait_event event;

  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_MUTEX_WAIT);
  event.lock = (__le32) lock;
  log_event(&event, sizeof(struct mutex_wait_event));
  local_irq_restore(flags);
#endif
}

void event_log_mutex_wake(void* lock) {
#ifdef CONFIG_EVENT_MUTEX_WAKE
  unsigned long flags;
  struct mutex_wake_event event;
  
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_MUTEX_WAKE);
  event.lock = (__le32) lock;
  log_event(&event, sizeof(struct mutex_wake_event));
  local_irq_restore(flags);
#endif
}

void event_log_mutex_notify(void* lock, pid_t pid) {
#ifdef CONFIG_EVENT_MUTEX_WAKE
  unsigned long flags;
  struct mutex_notify_event event;
  
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_MUTEX_NOTIFY);
  event.lock = (__le32) lock;
  event.pid = pid;
  log_event(&event, sizeof(struct mutex_notify_event));
  local_irq_restore(flags);
#endif
}

void event_log_sem_lock(void* lock) {
#ifdef CONFIG_EVENT_SEMAPHORE_LOCK
  unsigned long flags;
  struct sem_lock_event event;

  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_SEMAPHORE_LOCK);
  event.lock = (__le32) lock;
  log_event(&event, sizeof(struct sem_lock_event));
  local_irq_restore(flags);
#endif
}

void event_log_sem_wait(void* lock) {
#ifdef CONFIG_EVENT_SEMAPHORE_WAIT
  unsigned long flags;
  struct sem_wait_event event;

  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_SEMAPHORE_WAIT);
  event.lock = (__le32) lock;
  log_event(&event, sizeof(struct sem_wait_event));
  local_irq_restore(flags);
#endif
}
