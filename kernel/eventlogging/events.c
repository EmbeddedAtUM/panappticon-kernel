#include <eventlogging/events.h>

void event_log_waitqueue_wait(void* wq) {
#ifdef CONFIG_EVENT_WAITQUEUE_WAIT
  init_event(struct waitqueue_wait_event, EVENT_WAITQUEUE_WAIT, event);
  event->wq = (__le32) wq;
  finish_event();
#endif
}

void event_log_waitqueue_wake(void* wq) {
#ifdef CONFIG_EVENT_WAITQUEUE_WAKE
  init_event(struct waitqueue_wake_event, EVENT_WAITQUEUE_WAKE, event);
  event->wq = (__le32) wq;
  finish_event();
#endif
}

void event_log_waitqueue_notify(void* wq, pid_t pid) {
#ifdef CONFIG_EVENT_WAITQUEUE_NOTIFY
  init_event(struct waitqueue_notify_event, EVENT_WAITQUEUE_NOTIFY, event);
  event->wq = (__le32) wq;
  event->pid = pid;
  finish_event();
#endif
}
