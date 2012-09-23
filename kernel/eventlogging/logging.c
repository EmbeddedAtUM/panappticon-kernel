#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/gfp.h>

#include "buffer.h"
#include "proc_fs.h"

#define BUFFER_ORDER 12 // 2^12 = 16 MB with 4096 page size

static DEFINE_PER_CPU(struct dbuffer, buffers);

static __init int init_alloc_buffers(void) {
  int ret = 0;

  int cpu;
  for_each_cpu(cpu, cpu_possible_mask) {
    ret = dbuffer_init(&per_cpu(buffers, cpu), BUFFER_ORDER);
    if (ret)
      goto err;

    printk("eventlogging: Allocated buffer for CPU %d\n", cpu);
  }
  return 0;

 err:
  printk("eventlogging: Failed to allocate buffer for CPU %d\n", cpu);
  return ret;
}

static __init int init_proc_fs(void) {
  return event_logging_create_pfs();
}

early_initcall(init_alloc_buffers);
fs_initcall(init_proc_fs);
