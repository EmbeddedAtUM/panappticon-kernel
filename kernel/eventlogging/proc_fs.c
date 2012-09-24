#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/list.h>

#include "buffer.h"
#include "proc_fs.h"


#define PFS_NAME "event_logging"
#define PFS_PERMS S_IFREG|S_IROTH|S_IRGRP|S_IRUSR
static struct proc_dir_entry* el_pfs_entry;

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
  int len;
  struct sbuffer* buf;

  len = 0;
  *start = page;
  *eof = 1;

  while (len == 0) {
    buf = peek_full_blocking();
    if (buf == NULL)
      return -ERESTARTSYS;

    if (sbuffer_empty(buf)) 
      recycle_if_empty();
    else 
      len += sbuffer_read(buf, page, count);
  }
  
  return len;
}
