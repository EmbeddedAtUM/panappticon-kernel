#include <linux/time.h>
#include <linux/sched.h>
#include <linux/smp.h>

#include <eventlogging/events.h>
#include "logging.h"

void event_log_header_init(struct event_hdr* event, u8 type) {
  struct timeval tv;
  do_gettimeofday(&tv);

  event->event_type = type;
  event->tv_sec = tv.tv_sec;
  event->tv_usec = tv.tv_usec;
  event->cpu = smp_processor_id();
  event->pid = current->pid;
}

void event_log_simple(u8 event_type) {
  unsigned long flags;
  struct event_hdr event;
  local_irq_save(flags);
  event_log_header_init(&event, event_type);
  log_event(&event, sizeof(struct event_hdr));
  local_irq_restore(flags);
}

void event_log_context_switch(pid_t old, pid_t new) {
  unsigned long flags;
  struct context_switch_event event;
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_CONTEXT_SWITCH);
  event.old_pid = old;
  event.new_pid = new;
  log_event(&event, sizeof(struct context_switch_event));
  local_irq_restore(flags);
}

void event_log_network_block(void) {
  event_log_simple(EVENT_NETWORK_BLOCK);
}

void event_log_network_resume(void) {
  event_log_simple(EVENT_NETWORK_RESUME);
}

void event_log_fork(pid_t pid, pid_t tgid) {
  unsigned long flags;
  struct fork_event event;
  
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_FORK);
  event.pid = pid;
  event.tgid = tgid;
  log_event(&event, sizeof(struct fork_event));
  local_irq_restore(flags);
}
