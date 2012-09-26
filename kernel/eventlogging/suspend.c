#include <linux/suspend.h>
#include <linux/notifier.h>

#include <eventlogging/events.h>

#if defined(CONFIG_EVENT_SUSPEND_PREPARE) || defined(CONFIG_EVENT_POST_SUSPEND)

static int suspend_notifier_call(struct notifier_block* self, unsigned long event, void* empty) {
  switch (event) {
  case PM_SUSPEND_PREPARE:
    event_log_suspend_prepare();
    break;
  case PM_POST_SUSPEND:
    event_log_post_suspend();
    break;
  }
  return NOTIFY_OK;
}

static struct notifier_block suspend_notifier = {
  .notifier_call = suspend_notifier_call
};

__init int init_suspend_notifier(void) {
  register_pm_notifier(&suspend_notifier);
  return 0;
}
#else
__init int init_suspend_notifier(void){
  return 0;
}
#endif
