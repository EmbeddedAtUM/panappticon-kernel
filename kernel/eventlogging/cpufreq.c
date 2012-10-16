#include <linux/cpufreq.h>
#include <linux/notifier.h>

#include <eventlogging/events.h>

#ifdef CONFIG_EVENT_CPUFREQ_SET
static struct notifier_block cpufreq_notifier;

static int cpufreq_notifier_call(struct notifier_block* self, unsigned long event, void* data) {
  struct cpufreq_freqs* freqs;
  switch (event) {
  case CPUFREQ_POSTCHANGE:
    freqs = (struct cpufreq_freqs*) data;
    event_log_cpufreq_set(freqs->cpu, freqs->old, freqs->new);
    break;
  default:
    break;
  }
  return 0;
}

__init int init_cpufreq_notifier(void) {
  cpufreq_notifier.notifier_call = cpufreq_notifier_call;
  cpufreq_register_notifier(&cpufreq_notifier, CPUFREQ_TRANSITION_NOTIFIER);
  return 0;
}
#else
__init int init_cpufreq_notifier(void) {return 0;}
#endif
