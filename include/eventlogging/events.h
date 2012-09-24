#ifndef EVENTLOGGING_EVENTS_H
#define EVENTLOGGING_EVENTS_H

#include <linux/types.h>

#define EVENT_CONTEXT_SWITCH 10
#define EVENT_FORK 15

#define EVENT_IO_BLOCK 20
#define EVENT_IO_RESUME 21

#define EVENT_DATAGRAM_BLOCK 30
#define EVENT_DATAGRAM_RESUME 31
#define EVENT_STREAM_BLOCK 32
#define EVENT_STREAM_RESUME 33
#define EVENT_SOCK_BLOCK 34
#define EVENT_SOCK_RESUME 35

#define EVENT_SEMAPHORE_LOCK 40
#define EVENT_SEMAPHORE_WAIT 41

#define EVENT_MUTEX_LOCK 50
#define EVENT_MUTEX_WAIT 51

#define EVENT_IPC_LOCK 60
#define EVENT_IPC_WAIT 11

struct event_hdr {
  __u8  event_type;
  __u8  cpu;
  __le16 pid;
  __le32 tv_sec;
  __le32 tv_usec;
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

struct mutex_lock_event {
  struct event_hdr hdr;
  __le32 lock;
}__attribute__((packed));

struct mutex_wait_event {
  struct event_hdr hdr;
  __le32 lock;
}__attribute__((packed));

struct sem_lock_event {
  struct event_hdr hdr;
  __le32 lock;
}__attribute__((packed));

struct sem_wait_event {
  struct event_hdr hdr;
  __le32 lock;
}__attribute__((packed));

#ifdef __KERNEL__
void event_log_header_init(struct event_hdr* event, u8 type);
void event_log_simple(u8 event_type);
void event_log_context_switch(pid_t old, pid_t new);
void event_log_datagram_block(void);
void event_log_datagram_resume(void);
void event_log_stream_block(void);
void event_log_stream_resume(void);
void event_log_sock_block(void);
void event_log_sock_resume(void);
void event_log_fork(pid_t pid, pid_t tgid);
void event_log_mutex_lock(void* lock);
void event_log_mutex_wait(void* lock);;
void event_log_sem_lock(void* lock);
void event_log_sem_wait(void* lock);
#endif

#endif
