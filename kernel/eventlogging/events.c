#include <eventlogging/events.h>

void event_log_waitqueue_wait(void* wq) {
#ifdef CONFIG_EVENT_WAITQUEUE_WAIT
  unsigned long flags;
  struct waitqueue_wait_event event;
  
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_WAITQUEUE_WAIT);
  event.wq = (__le32) wq;
  log_event(&event, sizeof(struct waitqueue_wait_event));
  local_irq_restore(flags);
#endif
}

void event_log_waitqueue_wake(void* wq) {
#ifdef CONFIG_EVENT_WAITQUEUE_WAKE
  unsigned long flags;
  struct waitqueue_wake_event event;
  
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_WAITQUEUE_WAKE);
  event.wq = (__le32) wq;
  log_event(&event, sizeof(struct waitqueue_wake_event));
  local_irq_restore(flags);
#endif
}

void event_log_waitqueue_notify(void* wq, pid_t pid) {
#ifdef CONFIG_EVENT_WAITQUEUE_NOTIFY
  unsigned long flags;
  struct waitqueue_notify_event event;
  
  local_irq_save(flags);
  event_log_header_init(&event.hdr, EVENT_WAITQUEUE_NOTIFY);
  event.wq = (__le32) wq;
  event.pid = pid;
  log_event(&event, sizeof(struct waitqueue_notify_event));
  local_irq_restore(flags);
#endif
}
