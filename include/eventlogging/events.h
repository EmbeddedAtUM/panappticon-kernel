#ifndef EVENTLOGGING_EVENTS_H
#define EVENTLOGGING_EVENTS_H

#include <linux/types.h>

#define EVENT_CONTEXT_SWITCH 10
#define EVENT_FORK 15

#define EVENT_IO_BLOCK 20
#define EVENT_IO_RESUME 21

#define EVENT_NETWORK_BLOCK 30
#define EVENT_NETWORK_RESUME 31

#define EVENT_SEMAPHORE_LOCK 40
#define EVENT_SEMAPHORE_WAIT 41

#define EVENT_MUTEX_LOCK 50
#define EVENT_MUTEX_WAIT 51

#define EVENT_IPC_LOCK 60
#define EVENT_IPC_WAIT 11

struct event_hdr {
  __u8  event_type;
  __u8  cpu;
  __le32 tv_sec;
  __le32 tv_usec;
  __le16 pid;
}__attribute__((packed));

struct context_switch_event {
  struct event_hdr hdr;
  __le16 old_pid;
  __le16 new_pid;
}__attribute__((packed));

struct fork_event {
  struct event_hdr hdr;
  __le16 pid;
  __le16 tgid;
}__attribute__((packed));

struct network_block_event {
  struct event_hdr hdr;
}__attribute__((packed));

struct network_resume_event {
  struct event_hdr hdr;
}__attribute__((packed));

#ifdef __KERNEL__
void event_log_header_init(struct event_hdr* event, u8 type);
void event_log_simple(u8 event_type);
void event_log_context_switch(pid_t old, pid_t new);
void event_log_network_block(void);
void event_log_network_resume(void);
void event_log_fork(pid_t pid, pid_t tgid);
#endif

#endif
