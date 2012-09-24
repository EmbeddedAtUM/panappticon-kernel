#ifndef EVENT_LOGGING_BUFFER_H
#define EVENT_LOGGING_BUFFER_H

#include <linux/list.h>

struct sbuffer {
  struct list_head list;
  int order;   // order of page allocation (2^order pages)
  void* start; // starting address
  void* end;   // last address in buffer
  void* rp;    // pointer to next byte to read
  void* wp;    // pointer to next byte writer
};

void sbuffer_print_empty(void);

int sbuffer_init(struct sbuffer* buf, unsigned int order);
void sbuffer_free(struct sbuffer* buf);

void sbuffer_clear(struct sbuffer* buf);
int sbuffer_avail(struct sbuffer* buf); // how much space to write?
int sbuffer_write(struct sbuffer* buf, char* page, int count);
int sbuffer_empty(struct sbuffer* buf); // any data to read?
int sbuffer_read(struct sbuffer* buf, char* page, int count);


void put_empty(struct sbuffer* buf);
void put_full(struct sbuffer* buf);

struct sbuffer* take_empty_try(void);
struct sbuffer* peek_full_blocking(void);
void recycle_if_empty(void); // move from full to empty, if empty

#endif
