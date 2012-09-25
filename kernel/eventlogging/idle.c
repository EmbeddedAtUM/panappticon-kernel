#include <linux/cpu.h>
#include <linux/notifier.h>

#include <eventlogging/events.h>

#if defined(CONFIG_EVENT_IDLE_START) || defined(CONFIG_EVENT_IDLE_STOP)
static struct notifier_block idle_notifier;

static int idle_notifier_call(struct notifier_block* self, unsigned long event, void* data) {
  switch (event) {
  case IDLE_START:
    event_log_idle_start();
    break;
  case IDLE_END:
    event_log_idle_end();
    break;
  }
  return 0;
}

__init int init_idle_notifier(void) {
  idle_notifier.notifier_call = idle_notifier_call;
  idle_notifier_register(&idle_notifier);
  return 0;
}
#else
__init int init_idle_notifier(void) {}
#endif
