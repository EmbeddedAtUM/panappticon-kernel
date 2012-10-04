#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

#include <eventlogging/events.h>

#include "logging.h"
#include "buffer.h"
#include "idle.h"
#include "hotcpu.h"
#include "queue.h"

#define BUFFER_ORDER 10  // 2^10 = 4 MB with 4096 page size
#define NUM_BUFFERS  32  // 32 * 4 MB = 128 MB total

static DEFINE_PER_CPU(struct sbuffer*, cpu_buffers);
static DEFINE_PER_CPU(unsigned int, missed_events);

/* Timestamp from last packet */
static DEFINE_PER_CPU(struct timeval, last_tv);

static DEFINE_QUEUE(empty_buffers);
static DEFINE_QUEUE(full_buffers);

#define PFS_NAME "event_logging"
#define PFS_PERMS S_IFREG|S_IROTH|S_IRGRP|S_IRUSR|S_IWOTH|S_IWGRP|S_IWUSR
static struct proc_dir_entry* el_pfs_entry;
static DEFINE_MUTEX(pfs_read_lock);
static struct sbuffer* pfs_read_buffer;

static void init_new_buffer(void) {
  event_log_sync();
  if  (__get_cpu_var(missed_events) > 0)
       event_log_missed_count(&__get_cpu_var(missed_events));
}

inline static struct sbuffer* __get_new_cpu_buffer(void) {
  struct sbuffer* buf = queue_take_try(&empty_buffers);
  if (NULL == buf) 
    goto out;
  __get_cpu_var(cpu_buffers) = buf;
  init_new_buffer();
 out:
  return buf;
}

inline static struct sbuffer* __get_cpu_buffer(void) {
  struct sbuffer* buf = __get_cpu_var(cpu_buffers);
  if (NULL == buf) 
    buf = __get_new_cpu_buffer();
  return buf;
}

static struct sbuffer* __flush_cpu_buffer(void) {
  struct sbuffer* buf = __get_cpu_var(cpu_buffers);
  if (NULL != buf)
    queue_put(&full_buffers, buf);
  __get_cpu_var(cpu_buffers) = NULL;
  return __get_cpu_buffer();
}

/* If not enough space, returns NULL and logs a missed event. */
void* reserve_event(int len) {
  struct sbuffer* buf;
  void* wp;

  /* Get buffer, if available */
  buf = __get_cpu_buffer();
 check_buffer:
  if (!buf) {
    __get_cpu_var(missed_events)++;
    return NULL;
  }

  wp = sbuffer_reserve(buf, len);
  /* if full, get new buffer */
  if (!wp) {
    buf = __flush_cpu_buffer();
    goto check_buffer;
  }

  return wp;
}

void poke_queues(void) {
  queue_poke(&full_buffers);
}

void shrink_event(int len) {
  struct sbuffer* buf;
  buf = __get_cpu_buffer();
  if (buf)
    sbuffer_cancel(buf, len);
}

/* Returns a reference to the per-cpu timestamp of the last record */
struct timeval* get_timestamp(void) {
  return &__get_cpu_var(last_tv);
}

/*
 * Must be called with hotplugging disabled
 * and 'cpu' offline.
 */
static void __flush_offline_cpu_buffer(int cpu) {
  struct sbuffer* buf = per_cpu(cpu_buffers, cpu);
  printk("eventlogging: flushing offline cpu: %d\n", cpu);
  if (NULL == buf)
    return;
  queue_put(&full_buffers, buf); 
  per_cpu(cpu_buffers, cpu) = NULL;
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
    queue_put(&empty_buffers, buf);
  }
  printk("eventlogging: allocated %d buffers\n", cnt);

  /* Set up CPUs to grab new buffer on first event */
  for_each_cpu(cpu, cpu_possible_mask) {
    per_cpu(cpu_buffers, cpu) = NULL; 
    printk("eventlogging: prepare buffer for CPU %d\n", cpu);
  }

  return 0;
}

/* =========================== Proc FS Methods ============================== */

static int event_logging_read_pfs(char* page, char** start, off_t off, int count, int* eof, void* data) {
  int err, len;

  len = 0;
  *start = page;
  *eof = 1;

  err = mutex_lock_interruptible(&pfs_read_lock);
  if (err)
    goto err;

  while (len == 0) {
    /* Return now-empty buffer to empty queue */
    if (NULL != pfs_read_buffer && sbuffer_empty(pfs_read_buffer)) {
      sbuffer_clear(pfs_read_buffer);
      queue_put(&empty_buffers, pfs_read_buffer);
      pfs_read_buffer = NULL;
    }

    /* Get a new buffer from the full queue */
    if (NULL == pfs_read_buffer) {
      pfs_read_buffer = queue_take_interruptible(&full_buffers);
      if (IS_ERR(pfs_read_buffer)) {
	err = PTR_ERR(pfs_read_buffer);
	pfs_read_buffer = NULL;
	goto err;
      }
    }
    
    /* Read from the buffer */
    len += sbuffer_read(pfs_read_buffer, page, count);
  }
  
  mutex_unlock(&pfs_read_lock);
  return len;

  err:
    mutex_unlock(&pfs_read_lock);
    return err;
}

static int event_logging_write_pfs(struct file* file, const char* buffer, unsigned long count, void *data) {
  if (count > 0)
    flush_all_cpus();
  return count;
}

static __init int event_logging_create_pfs(void) {
  el_pfs_entry = create_proc_entry(PFS_NAME, PFS_PERMS, NULL);
  if (!el_pfs_entry)
    goto err;
  
  el_pfs_entry->uid = 0;
  el_pfs_entry->gid = 0;
  el_pfs_entry->read_proc = event_logging_read_pfs;
  el_pfs_entry->write_proc = event_logging_write_pfs;
  return 0;

 err:
  return -EINVAL;
}

/* ========================= Initialization Config ========================== */

early_initcall(init_alloc_buffers);
early_initcall(init_idle_notifier);
early_initcall(init_hotcpu_notifier);
fs_initcall(event_logging_create_pfs);


