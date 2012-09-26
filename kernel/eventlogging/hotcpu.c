#include <linux/cpu.h>
#include <linux/notifier.h>

#include <eventlogging/events.h>

#if defined(CONFIG_EVENT_CPU_ONLINE) || defined(CONFIG_EVENT_CPU_DEAD) || defined(CONFIG_EVENT_CPU_DOWN_PREPARE)

static int hotcpu_notifier_call(struct notifier_block* self, unsigned long event, void* hcpu) {
  unsigned int cpu = (unsigned long) hcpu;

  switch (event) {
  case CPU_ONLINE:
  case CPU_ONLINE_FROZEN:
    event_log_cpu_online(cpu);
    break;
  case CPU_DOWN_PREPARE:
  case CPU_DOWN_PREPARE_FROZEN:
    event_log_cpu_down_prepare(cpu);
    break;
  case CPU_DEAD:
  case CPU_DEAD_FROZEN:
    event_log_cpu_dead(cpu);
    break;
  }
  return NOTIFY_OK;
}

static struct notifier_block hotcpu_notifier = {
  .notifier_call = hotcpu_notifier_call
};

__init int init_hotcpu_notifier(void) {
  register_cpu_notifier(&hotcpu_notifier);
  return 0;
}
#else
__init int init_hotcpu_notifier(void){
  return 0;
}
#endif
