#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/slab.h>

#include "logging.h"
#include "buffer.h"
#include "proc_fs.h"

// 2^12 = 16 MB with 4096 page size
#define BUFFER_ORDER 4

static DEFINE_PER_CPU(struct sbuffer*, sbuffers);
static DEFINE_PER_CPU(unsigned int, missed_events);

static struct sbuffer* __get_cpu_buffer(void) {
  struct sbuffer* buf = __get_cpu_var(sbuffers);
  if (NULL == buf) {
    buf = take_empty_try();
    __get_cpu_var(sbuffers) = buf;
  }
  return buf;
}

static struct sbuffer* __flush_cpu_buffer(void) {
  put_full(__get_cpu_var(sbuffers));
  __get_cpu_var(sbuffers) = NULL;
  return __get_cpu_buffer();
}

void log_event(void* data, int len) {
  struct sbuffer* buf;
  unsigned long avail;

  // Get buffer, if available
  buf = __get_cpu_buffer();
  if (NULL == buf) {
    __get_cpu_var(missed_events)++;
    return;
  }

  // Flush and replace buffer, if not enough room for new event
  avail = sbuffer_avail(buf);
  if (avail < len) {
    buf = __flush_cpu_buffer();
    if (NULL == buf) {
      __get_cpu_var(missed_events)++;
      return;
    }
  }
  
  // Write data
  sbuffer_write(buf, data, len);
}

/*
 * Must be called with hotplugging disabled
 * and 'cpu' offline.
 */
static void __flush_offline_cpu_buffer(int cpu) {
  printk("eventlogging: flushing offline cpu: %d\n", cpu);
  put_full(per_cpu(sbuffers, cpu));
  per_cpu(sbuffers, cpu) = take_empty_try();
}

/*
 * Must be called with hotplugging disabled.
 */
static void __flush_offline_cpus(void) {
  int cpu;
  for_each_cpu_not(cpu, cpu_online_mask) {
    __flush_offline_cpu_buffer(cpu);
  }
}

static void __flush_online_cpu(void* info) {
  preempt_disable();
  printk("eventlogging: flushing online cpu: %d\n", smp_processor_id());
  __flush_cpu_buffer();
  preempt_enable();
}

/*
 * Might sleep, so must be called in sleepable context.
 */
void flush_all_cpus(void) {
  get_online_cpus(); // Disable hotplugging
  preempt_disable();

  on_each_cpu(__flush_online_cpu, NULL, 1); // Only runs on online cpus
  __flush_offline_cpus();

  preempt_enable();
  put_online_cpus(); // Enable hotplugging
}

static __init int init_alloc_buffers(void) {
  int ret = 0;

  int cpu;
  for_each_cpu(cpu, cpu_possible_mask) {
    // Create two buffers, one to use now and one for the empty list
    int i;
    struct sbuffer* buf[2];
    for (i = 0; i < 2; ++i) {
      buf[i] = (struct sbuffer*) kmalloc(sizeof(struct sbuffer), GFP_ATOMIC);
      if (0 == buf[i]) {
	ret = -ENOMEM;
	goto err;
      }
      sbuffer_init(buf[i], BUFFER_ORDER);
    }

    // Attach buffers
    per_cpu(sbuffers, cpu) = buf[0];
    put_empty(buf[1]);

    printk("eventlogging: allocated buffers for CPU %d\n", cpu);
  }

  return 0;

 err:
  printk("eventlogging: failed to allocate buffer for CPU %d\n", cpu);
  return ret;
}

static __init int init_proc_fs(void) {
  return event_logging_create_pfs();
}

early_initcall(init_alloc_buffers);
fs_initcall(init_proc_fs);
