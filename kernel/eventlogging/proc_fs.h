#ifndef EVENT_LOGGING_PROC_FS_H
#define EVENT_LOGGING_PROC_FS_H

#include "buffer.h"

int event_logging_create_pfs(void);
void event_logging_remove_pfs(void);
int event_logging_read_pfs(char* page, char** start, off_t off, int count, int* eof, void* data);

#endif
