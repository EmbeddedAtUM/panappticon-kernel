#ifndef EVENTLOGGING_EVENTS_H
#define EVENTLOGGING_EVENTS_H

#include <linux/types.h>

#define EVENT_LOG_MAGIC "michigan"

#define EVENT_SYNC_LOG 0

#define EVENT_CONTEXT_SWITCH 10
#define EVENT_FORK 15
#define EVENT_THREAD_NAME 16

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
#define EVENT_MUTEX_WAKE 52
#define EVENT_MUTEX_NOTIFY 53

#define EVENT_IPC_LOCK 60
#define EVENT_IPC_WAIT 11

struct event_hdr {
  __u8  event_type;
  __u8  cpu;
  __le16 pid;
  __le32 tv_sec;
  __le32 tv_usec;
}__attribute__((packed));

struct sync_log_event {
  struct event_hdr hdr;
  char magic[8];
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

struct thread_name_event {
  struct event_hdr hdr;
  char comm[16];
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

struct mutex_wake_event {
  struct event_hdr hdr;
  __le32 lock;
}__attribute__((packed));

struct mutex_notify_event {
  struct event_hdr hdr;
  __le32 lock;
  __le16 pid;
}__attribute__((packed));

struct sem_lock_event {
  struct event_hdr hdr;
  __le32 lock;
}__attribute__((packed));

struct sem_wait_event {
  struct event_hdr hdr;
  __le32 lock;
}__attribute__((packed));

struct io_block_event {
  struct event_hdr hdr;
}__attribute__((packed));

struct io_resume_event {
  struct event_hdr hdr;
}__attribute__((packed));

#ifdef __KERNEL__

#ifdef CONFIG_EVENT_LOGGING
#define DEFINE_EVENT_LOG_FUNC(name, ...) void event_log_##name(__VA_ARGS__)
#else
#define DEFINE_EVENT_LOG_FUNC()name, ...) void event_log_##name(__VA_ARGS__){}
#endif

DEFINE_EVENT_LOG_FUNC(context_switch, pid_t old, pid_t new);
DEFINE_EVENT_LOG_FUNC(datagram_block, void);
DEFINE_EVENT_LOG_FUNC(datagram_resume, void);
DEFINE_EVENT_LOG_FUNC(stream_block, void);
DEFINE_EVENT_LOG_FUNC(stream_resume, void);
DEFINE_EVENT_LOG_FUNC(sock_block, void);
DEFINE_EVENT_LOG_FUNC(sock_resume, void);
DEFINE_EVENT_LOG_FUNC(fork, pid_t pid, pid_t tgid);
DEFINE_EVENT_LOG_FUNC(thread_name, struct task_struct* task);
DEFINE_EVENT_LOG_FUNC(mutex_lock, void* lock);
DEFINE_EVENT_LOG_FUNC(mutex_wait, void* lock);
DEFINE_EVENT_LOG_FUNC(mutex_wake, void* lock);
DEFINE_EVENT_LOG_FUNC(mutex_notify, void* lock, pid_t pid);
DEFINE_EVENT_LOG_FUNC(sem_lock, void* lock);
DEFINE_EVENT_LOG_FUNC(sem_wait, void* lock);
DEFINE_EVENT_LOG_FUNC(io_block, void);
DEFINE_EVENT_LOG_FUNC(io_resume, void);

#endif // __KERNEL__
#endif // EVENTLOGGING_EVENTS_H
