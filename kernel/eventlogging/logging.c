#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/lzo.h>

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
static DEFINE_QUEUE(compressed_buffers);

#define PFS_NAME "event_logging"
#define PFS_PERMS S_IFREG|S_IROTH|S_IRGRP|S_IRUSR|S_IWOTH|S_IWGRP|S_IWUSR
static struct proc_dir_entry* el_pfs_entry;
static DEFINE_MUTEX(pfs_read_lock);
static struct sbuffer* pfs_read_buffer;

static DEFINE_MUTEX(compress_lock);
static struct sbuffer* compress_empty_buffer;

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

static void schedule_compression(void);

void poke_queues(void) {
  schedule_compression();
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

  /* Allocate empty buffer for compression */
  compress_empty_buffer = queue_take_try(&empty_buffers);
  if (!compress_empty_buffer)
    printk(KERN_ERR "eventlogging: failed to allocate empty buffer for compression\n");

  return 0;
}

/* ============================= Compression ================================ */
static char lzo_work_mem[LZO1X_1_MEM_COMPRESS];

static int compress_buffer(struct sbuffer* buf) {
  int err;
  u32 compressed_len;

  err = mutex_lock_interruptible(&compress_lock);
  if (err)
    return err;

  /* Try to get empty buffer, if one is not already available. This
     should never happen. */
  if (!compress_empty_buffer && 
      !(compress_empty_buffer = queue_take_try(&empty_buffers)))
    goto out;

  sbuffer_clear(compress_empty_buffer);

  /* Reserve four bytes to record data size */
  compress_empty_buffer->wp += 4;

  compressed_len = compress_empty_buffer->end - compress_empty_buffer->start;
  err = lzo1x_1_compress(buf->rp, (buf->wp - buf->rp), compress_empty_buffer->wp, &compressed_len, &lzo_work_mem);
  if (err) {
    printk(KERN_ERR "eventlogging: error compressing buffer: %d", err);
    goto out;
  }
  compress_empty_buffer->wp += compressed_len;
  memcpy(compress_empty_buffer->start, &compressed_len, 4);

  sbuffer_swap(compress_empty_buffer, buf);
  err = 0;

 out:
  mutex_unlock(&compress_lock);
  return err;
}

static void compress_buffer_func(struct work_struct* work) {
  int ret;
  struct sbuffer* buf;
  
  buf = container_of(work, struct sbuffer, work);
  ret = compress_buffer(buf);  
  if (ret)
    goto err;

  queue_put(&compressed_buffers, buf);
  queue_poke(&compressed_buffers);
  return;

 err:
  printk("eventlogging: failed to compress buffer: %d", ret);
}

static void schedule_compression(void) {
  struct sbuffer* buf;
  while( (buf = queue_take_try(&full_buffers)) ) {
    INIT_WORK(&buf->work, compress_buffer_func);
    schedule_work(&buf->work);
  }
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
      pfs_read_buffer = queue_take_interruptible(&compressed_buffers);
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


