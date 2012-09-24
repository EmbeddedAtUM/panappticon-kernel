#ifndef EVENT_LOGGING_H
#define EVENT_LOGGING_H

#include "buffer.h"

void flush_all_cpus(void);
void log_event(void* data, int len);

#endif
