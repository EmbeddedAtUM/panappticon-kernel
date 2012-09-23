#ifndef EVENT_LOGGING_PROC_FS_H
#define EVENT_LOGGING_PROC_FS_H

int event_logging_create_pfs(void);
void event_logging_remove_pfs(void);
int event_logging_read_pfs(char* buffer, char** start, off_t offset, int len, int* eof, void* data);

#endif
