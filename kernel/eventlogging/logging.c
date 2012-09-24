#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/slab.h>

#include <eventlogging/events.h>

#include "logging.h"
#include "buffer.h"
#include "proc_fs.h"

extern void event_log_header_init(struct event_hdr* event, u8 type);

#define BUFFER_ORDER   10  // 2^10 = 4 MB with 4096 page size
#define NUM_BUFFERS 32  // 32 * 4 MB = 128 MB total

static DEFINE_PER_CPU(struct sbuffer*, sbuffers);
static DEFINE_PER_CPU(unsigned int, missed_events);

inline static void log_sync_event(struct sbuffer* buf) {
  struct sync_log_event event;
  event_log_header_init(&event.hdr, EVENT_SYNC_LOG);
  memcpy(&event.magic, EVENT_LOG_MAGIC, 8);
  sbuffer_write(buf, (void*)&event, sizeof(struct sync_log_event));
}

inline static struct sbuffer* __get_new_cpu_buffer(void) {
  struct sbuffer* buf = take_empty_try();
  if (NULL != buf)
    log_sync_event(buf);
  return buf;
}

inline static struct sbuffer* __get_cpu_buffer(void) {
  struct sbuffer* buf = __get_cpu_var(sbuffers);
  if (NULL == buf) 
    buf = __get_new_cpu_buffer();
  __get_cpu_var(sbuffers) = buf;
  return buf;
}

static struct sbuffer* __flush_cpu_buffer(void) {
  struct sbuffer* buf = __get_cpu_var(sbuffers);
  if (NULL != buf)
    put_full(buf);
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
  struct sbuffer* buf = per_cpu(sbuffers, cpu);
  printk("eventlogging: flushing offline cpu: %d\n", cpu);
  if (NULL == buf)
    return;
  put_full(buf);
  per_cpu(sbuffers, cpu) = NULL;
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
  int i, cpu;
  int cnt = 0;
  
  /* Allocate all buffers */
  for(i = 0; i < NUM_BUFFERS; ++i) {
    struct sbuffer* buf;

    buf = (struct sbuffer*) kmalloc(sizeof(struct sbuffer), GFP_ATOMIC);
    if (0 == buf) {
      printk("eventlogging: failed to allocate buffer\n");
      continue;
    }

    ++cnt;
    sbuffer_init(buf, BUFFER_ORDER);
    put_empty(buf);
  }
  printk("eventlogging: allocated %d buffers\n", cnt);

  /* Set up CPUs to grab new buffer on first event */
  for_each_cpu(cpu, cpu_possible_mask) {
    per_cpu(sbuffers, cpu) = NULL; 
    printk("eventlogging: prepare buffer for CPU %d\n", cpu);
  }

  return 0;
}

static __init int init_proc_fs(void) {
  return event_logging_create_pfs();
}

early_initcall(init_alloc_buffers);
fs_initcall(init_proc_fs);
