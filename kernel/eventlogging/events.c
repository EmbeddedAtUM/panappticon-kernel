#include <eventlogging/events.h>

void event_log_waitqueue_wait(void* wq) {
#ifdef CONFIG_EVENT_WAITQUEUE_WAIT
  event_log_general_lock(EVENT_WAITQUEUE_WAIT, wq);
#endif
}

void event_log_waitqueue_wake(void* wq) {
#ifdef CONFIG_EVENT_WAITQUEUE_WAKE
  event_log_general_lock(EVENT_WAITQUEUE_WAKE, wq);
#endif
}

void event_log_waitqueue_notify(void* wq, pid_t pid) {
#ifdef CONFIG_EVENT_WAITQUEUE_NOTIFY
  event_log_general_notify(EVENT_WAITQUEUE_NOTIFY, wq, pid);
#endif
}
