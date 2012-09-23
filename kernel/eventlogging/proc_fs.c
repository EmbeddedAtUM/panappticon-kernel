#include <linux/proc_fs.h>
#include <linux/sched.h>
#include "proc_fs.h"

#define PFS_NAME "event_logging"
#define PFS_PERMS S_IFREG|S_IROTH|S_IRGRP|S_IRUSR
static struct proc_dir_entry* el_pfs_entry;

DECLARE_WAIT_QUEUE_HEAD(el_pfs_queue);

int event_logging_create_pfs(void) {
  el_pfs_entry = create_proc_entry(PFS_NAME, PFS_PERMS, NULL);
  if (!el_pfs_entry)
    goto err;
  
  el_pfs_entry->uid = 0;
  el_pfs_entry->gid = 0;
  el_pfs_entry->read_proc = event_logging_read_pfs;
  el_pfs_entry->write_proc = NULL;
  return 0;

 err:
  return -EINVAL;
}

void event_logging_remove_pfs(void) {
  remove_proc_entry(PFS_NAME, NULL);
}

int event_logging_read_pfs(char* page, char** start, off_t off, int count, int* eof, void* data) {
  int len = 0;

  *start = page;
  *eof = 1;

  while (len == 0) {
    if ( wait_event_interruptible(el_pfs_queue, 0) )
      return -ERESTARTSYS;

    len = 0;
  }

  return len;
}
