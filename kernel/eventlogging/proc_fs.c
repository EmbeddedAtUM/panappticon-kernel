#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>

#include "logging.h"
#include "buffer.h"
#include "queue.h"
#include "proc_fs.h"

#define PFS_NAME "event_logging"
#define PFS_PERMS S_IFREG|S_IROTH|S_IRGRP|S_IRUSR|S_IWOTH|S_IWGRP|S_IWUSR
static struct proc_dir_entry* el_pfs_entry;

extern struct queue empty_buffers;
extern struct queue full_buffers;

static DEFINE_MUTEX(read_procfs);
static struct sbuffer* read_buffer;

int event_logging_create_pfs(void) {
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

void event_logging_remove_pfs(void) {
  remove_proc_entry(PFS_NAME, NULL);
}

int event_logging_read_pfs(char* page, char** start, off_t off, int count, int* eof, void* data) {
  int err, len;

  len = 0;
  *start = page;
  *eof = 1;

  err = mutex_lock_interruptible(&read_procfs);
  if (err)
    goto err;

  while (len == 0) {
    /* Return now-empty buffer to empty queue */
    if (NULL != read_buffer && sbuffer_empty(read_buffer)) {
      sbuffer_clear(read_buffer);
      queue_put(&empty_buffers, read_buffer);
      read_buffer = NULL;
    }

    /* Get a new buffer from the full queue */
    if (NULL == read_buffer) {
      read_buffer = queue_take_interruptible(&full_buffers);
      if (IS_ERR(read_buffer)) {
	err = PTR_ERR(read_buffer);
	read_buffer = NULL;
	goto err;
      }
    }
    
    /* Read from the buffer */
    len += sbuffer_read(read_buffer, page, count);
  }
  
  mutex_unlock(&read_procfs);
  return len;

  err:
    mutex_unlock(&read_procfs);
    return err;
}

int event_logging_write_pfs(struct file* file, const char* buffer, unsigned long count, void *data) {
  if (count > 0)
    flush_all_cpus();
  return count;
}
