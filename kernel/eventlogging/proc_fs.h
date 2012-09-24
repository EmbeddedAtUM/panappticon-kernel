#ifndef EVENT_LOGGING_PROC_FS_H
#define EVENT_LOGGING_PROC_FS_H

#include <linux/proc_fs.h>
#include "buffer.h"

int event_logging_create_pfs(void);
void event_logging_remove_pfs(void);
int event_logging_read_pfs(char* page, char** start, off_t off, int count, int* eof, void* data);
int event_logging_write_pfs(struct file* file, const char* buffer, unsigned long count, void* data);

#endif
