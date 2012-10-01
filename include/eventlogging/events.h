#ifndef EVENTLOGGING_EVENTS_H
#define EVENTLOGGING_EVENTS_H

#include <linux/types.h>

#define EVENT_LOG_MAGIC "michigan"

#define EVENT_SYNC_LOG 0
#define EVENT_MISSED_COUNT 1

#define EVENT_CPU_ONLINE 5
#define EVENT_CPU_DOWN_PREPARE 6
#define EVENT_CPU_DEAD 7

#define EVENT_PREEMPT_WAKEUP 9
#define EVENT_CONTEXT_SWITCH 10
#define EVENT_PREEMPT_TICK 11
#define EVENT_YIELD 12

#define EVENT_IDLE_START 13
#define EVENT_IDLE_END 14
#define EVENT_FORK 15
#define EVENT_THREAD_NAME 16
#define EVENT_EXIT 17

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

#define EVENT_WAITQUEUE_WAIT 55
#define EVENT_WAITQUEUE_WAKE 56
#define EVENT_WAITQUEUE_NOTIFY 57

#define EVENT_IPC_LOCK 60
#define EVENT_IPC_WAIT 61

#define EVENT_WAKE_LOCK 70
#define EVENT_WAKE_UNLOCK 71

#define EVENT_SUSPEND_START 75
#define EVENT_SUSPEND 76
#define EVENT_RESUME 77
#define EVENT_RESUME_FINISH 78

#define MAX8 ((1 << 7) - 1)
#define MIN8 (-(1 << 7))

#define MAX16 ((1 << 15) - 1)
#define MIN16 (-(1 << 15))

#define MAX24 ((1 << 23) - 1)
#define MIN24 (-(1 << 23))

struct event_hdr {
  __u8 event_type;
  __u8 cpu_tvlen;
  __le16 pid;
}__attribute__((packed));

#define CPU_MASK = 0xF0
#define TVLEN_MASK = 0x0F

#define SET_CPU(header, val) header->cpu_tvlen = (header->cpu_tvlen & ~(0xF0)) | ((val) << 4)
#define GET_CPU(header) ((header->cpu_tvlen & 0xF0) >> 4)

#define SET_TVLEN(header, sec, usec) do {				\
    if ((sec) == 4)							\
      header->cpu_tvlen = (header->cpu_tvlen & ~(0x0F)) | ((usec) << 2); \
    else								\
      header->cpu_tvlen = (header->cpu_tvlen & ~(0x0F)) | ((sec) << 2) | (usec); \
  } while(0)

#define GET_SEC_LEN(header) ({				\
      int __ret;					\
      if (0 == (header->cpu_tvlen & 0x03))		\
	__ret = 4;					\
      else						\
	__ret = (header->cpu_tvlen & 0x0C) >> 2;	\
      __ret;						\
    })

#define GET_USEC_LEN(header) ({			\
  int __ret = (header->cpu_tvlen & 0x03);	\
  if (0 == __ret)				\
    __ret = (header->cpu_tvlen & 0x0C) >> 2;	\
  __ret;					\
})

struct sync_log_event {
  char magic[8];
}__attribute__((packed));

struct missed_count_event {
  __le32 count;
}__attribute__((packed));

struct context_switch_event {
  __le16 new_pid;
  __u8   state;  
}__attribute__((packed));

struct hotcpu_event {
  __u8 cpu;
}__attribute__((packed));

struct wake_lock_event {
  __le32 lock;
  __le32 timeout;
}__attribute__((packed));

struct wake_unlock_event {
  __le32 lock;
}__attribute__((packed));

struct fork_event {
  __le16 pid;
  __le16 tgid;
}__attribute__((packed));

struct thread_name_event {
  __u16 pid;
  char comm[16];
}__attribute__((packed));

struct waitqueue_wait_event {
  __le32 wq;
}__attribute__((packed));

struct waitqueue_wake_event {
  __le32 wq;
}__attribute__((packed));

struct waitqueue_notify_event {
  __le32 wq;
  __le16 pid;
}__attribute__((packed));

struct mutex_lock_event {
  __le32 lock;
}__attribute__((packed));

struct mutex_wait_event {
  __le32 lock;
}__attribute__((packed));

struct mutex_wake_event {
  __le32 lock;
}__attribute__((packed));

struct mutex_notify_event {
  __le32 lock;
  __le16 pid;
}__attribute__((packed));

struct sem_lock_event {
  __le32 lock;
}__attribute__((packed));

struct sem_wait_event {
  __le32 lock;
}__attribute__((packed));

struct simple_event {
}__attribute__((packed));

#ifdef __KERNEL__

#include <linux/hardirq.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/smp.h>

#ifdef CONFIG_EVENT_LOGGING
extern void* reserve_event(int len);
extern void shrink_event(int len);
extern struct timeval* get_timestamp(void);

#define __init_event(type, event_type, name, diff)			\
  struct timeval tv;							\
  u8 sec_len;								\
  u8 usec_len;								\
  struct event_hdr* header;						\
  char* sec;								\
  char* usec;								\
  type* name;								\
  unsigned long flags;							\
  local_irq_save(flags);						\
  header = (typeof(header)) reserve_event(sizeof(*header) + 4 + 3 + sizeof(*name)); \
  if (header) {								\
  tv = event_log_timestamp(diff);					\
  if (diff) {								\
    sec_len = vsize_sec(tv.tv_sec);					\
    usec_len = vsize_usec(tv.tv_usec);					\
  } else {								\
    sec_len = 4;							\
    usec_len = 3;							\
  }									\
  shrink_event(4 + 3 - sec_len - usec_len);				\
  sec = (char*) (header+1);						\
  usec = sec + sec_len;							\
  name = (typeof(name)) (usec + usec_len);				\
  memcpy(sec, &tv.tv_sec, sec_len);					\
  memcpy(usec, &tv.tv_usec, usec_len);					\
  event_log_header_init(header, sec_len, usec_len, event_type)

#define init_event(type, event_type, name) __init_event(type, event_type, name, 1)

#define finish_event() } \
    local_irq_restore(flags)

/* Records the current timestamp and, if diff is true, returns the
 * time passed since the last timestamp. Otherwise, the recorded current
 * time is returned.  static inline struct timeval */
static inline struct timeval event_log_timestamp(int diff) {
  struct timeval *cur = get_timestamp();
  struct timeval prev = *cur;
  do_gettimeofday(cur);
  if (!diff) 
    return *cur;
  prev.tv_sec = cur->tv_sec - prev.tv_sec;
  prev.tv_usec = cur->tv_usec - prev.tv_usec;
  return prev;
}

static inline u8 vsize_usec(long val) {
  if ((MIN8 <= val) && (val <= MAX8))
    return 1;
  else if ((MIN16 <= val) && (val <= MAX16))
    return 2;
  else
    return 3;
}

static inline u8 vsize_sec(long val) {
  if (val == 0)
    return 0;
  else if ((MIN8 <= val) && (val <= MAX8))
    return 1;
  else if ((MIN16 <= val) && (val <= MAX16))
    return 2;
  else if ((MIN24 <= val) && (val <= MAX24))
    return 3;
  else
    return 4;
}

static inline void event_log_header_init(struct event_hdr* event, u8 sec_len, u8 usec_len, u8 type) {
  event->event_type = type;
  SET_CPU(event, smp_processor_id());
  SET_TVLEN(event, sec_len, usec_len);
  event->pid = current->pid | (in_interrupt() ? 0x8000 : 0);
}

static inline void event_log_simple(u8 event_type) {
  init_event(struct simple_event, event_type, event);
  finish_event();
}

static inline void event_log_sync(void) {
  __init_event(struct sync_log_event, EVENT_SYNC_LOG, event, 0);
  memcpy(&event->magic, EVENT_LOG_MAGIC, 8);
  finish_event();
}

static inline void event_log_missed_count(int* count) {
  init_event(struct missed_count_event, EVENT_MISSED_COUNT, event);
  event->count = *count;
  *count = 0;
  finish_event();
}

#endif

static inline void event_log_context_switch(pid_t new, long state) {
#ifdef CONFIG_EVENT_CONTEXT_SWITCH
  init_event(struct context_switch_event, EVENT_CONTEXT_SWITCH, event);
  event->new_pid = new;
  event->state = (__u8) (0x0FF & state);
  finish_event();
#endif
}

static inline void event_log_preempt_tick(void) {
#ifdef CONFIG_EVENT_PREEMPT_TICK
  event_log_simple(EVENT_PREEMPT_TICK);
#endif
}

static inline void event_log_preempt_wakeup(void) {
#ifdef CONFIG_EVENT_PREEMPT_WAKEUP
  event_log_simple(EVENT_PREEMPT_WAKEUP);
#endif
}

static inline void event_log_yield(void) {
#ifdef CONFIG_EVENT_YIELD
  event_log_simple(EVENT_YIELD);
#endif
}

#if defined(CONFIG_EVENT_CPU_ONLINE) || defined(CONFIG_EVENT_CPU_DEAD) || defined(CONFIG_EVENT_CPU_DOWN_PREPARE)
static inline void event_log_hotcpu(unsigned int cpu, u8 event_type) {
  init_event(struct hotcpu_event, EVENT_CPU_ONLINE, event);
  event->cpu = cpu;
  finish_event();
}
#endif

static inline void event_log_cpu_online(unsigned int cpu) {
#ifdef CONFIG_EVENT_CPU_ONLINE
  event_log_hotcpu(cpu, EVENT_CPU_ONLINE);
#endif
}

static inline void event_log_cpu_down_prepare(unsigned int cpu) {
#ifdef CONFIG_EVENT_CPU_DOWN_PREPARE
  event_log_hotcpu(cpu, EVENT_CPU_DOWN_PREPARE);
#endif
}

static inline void event_log_cpu_dead(unsigned int cpu) {
#ifdef CONFIG_EVENT_CPU_DEAD
  event_log_hotcpu(cpu, EVENT_CPU_DEAD);
#endif
}

static inline void event_log_idle_start(void) {
#ifdef CONFIG_EVENT_IDLE_START
  event_log_simple(EVENT_IDLE_START);
#endif
}

static inline void event_log_idle_end(void) {
#ifdef CONFIG_EVENT_IDLE_END
  event_log_simple(EVENT_IDLE_END);
#endif
}

static inline void event_log_suspend_start(void) {
#ifdef CONFIG_EVENT_SUSPEND_START
  event_log_simple(EVENT_SUSPEND_START);
#endif
}

static inline void event_log_suspend(void) {
#ifdef CONFIG_EVENT_SUSPEND
  event_log_simple(EVENT_SUSPEND);
#endif
}

static inline void event_log_resume(void) {
#ifdef CONFIG_EVENT_RESUME
  event_log_simple(EVENT_RESUME);
#endif
}

static inline void event_log_resume_finish(void) {
#ifdef CONFIG_EVENT_RESUME_FINISH
  event_log_simple(EVENT_RESUME_FINISH);
#endif
}

static inline void event_log_datagram_block(void) {
#ifdef CONFIG_EVENT_DATAGRAM_BLOCK
  event_log_simple(EVENT_DATAGRAM_BLOCK);
#endif
}

static inline void event_log_datagram_resume(void) {
#ifdef CONFIG_EVENT_DATAGRAM_RESUME
  event_log_simple(EVENT_DATAGRAM_RESUME);
#endif
}

static inline void event_log_stream_block(void) {
#ifdef CONFIG_EVENT_STREAM_BLOCK
  event_log_simple(EVENT_STREAM_BLOCK);
#endif
}

static inline void event_log_stream_resume(void) {
#ifdef CONFIG_EVENT_STREAM_RESUME
  event_log_simple(EVENT_STREAM_RESUME);
#endif
}

static inline void event_log_sock_block(void) {
#ifdef CONFIG_EVENT_SOCK_BLOCK
  event_log_simple(EVENT_SOCK_BLOCK);
#endif
}

static inline void event_log_sock_resume(void) {
#ifdef CONFIG_EVENT_SOCK_RESUME
  event_log_simple(EVENT_SOCK_RESUME);
#endif
}

static inline void event_log_io_block(void) {
#ifdef CONFIG_EVENT_IO_BLOCK
  event_log_simple(EVENT_IO_BLOCK);
#endif
}

static inline void event_log_io_resume(void) {
#ifdef CONFIG_EVENT_IO_RESUME
  event_log_simple(EVENT_IO_RESUME);
#endif
}

static inline void event_log_wake_lock(void* lock, long timeout) {
#ifdef CONFIG_EVENT_WAKE_LOCK
  init_event(struct wake_lock_event, EVENT_WAKE_LOCK, event);
  event->lock = (__le32) lock;
  event->timeout = timeout;
  finish_event();
#endif
}

static inline void event_log_wake_unlock(void* lock) {
#ifdef CONFIG_EVENT_WAKE_UNLOCK
  init_event(struct wake_unlock_event, EVENT_WAKE_UNLOCK, event);
  event->lock = (__le32) lock;
  finish_event();
#endif
}

static inline void event_log_fork(pid_t pid, pid_t tgid) {
#ifdef CONFIG_EVENT_FORK
  init_event(struct fork_event, EVENT_FORK, event);
  event->pid = pid;
  event->tgid = tgid;
  finish_event();
#endif
}

static inline void event_log_exit(void) {
#ifdef CONFIG_EVENT_EXIT
  event_log_simple(EVENT_EXIT);
#endif
}

static inline void event_log_thread_name(struct task_struct* task) {
#ifdef CONFIG_EVENT_THREAD_NAME
  init_event(struct thread_name_event, EVENT_THREAD_NAME, event);
  event->pid = task->pid;
  memcpy(event->comm, task->comm, min(16, TASK_COMM_LEN));
  finish_event(); 
#endif
}

/* Can't be inlined due to #include ordering conflicts in wait.h and I
 * don't want to figure that out right now.  Can do it later, but
 * might involve a separate header file for the waitqueue events
 * or externing (instead of #including) the depedencies
 * for event_log_header_init().
 */
void event_log_waitqueue_wait(void* wq);
void event_log_waitqueue_wake(void* wq);
void event_log_waitqueue_notify(void* wq, pid_t pid);

static inline void event_log_mutex_lock(void* lock) {
#ifdef CONFIG_EVENT_MUTEX_LOCK
  init_event(struct mutex_lock_event, EVENT_MUTEX_LOCK, event);
  event->lock = (__le32) lock;
  finish_event();
#endif
}

static inline void event_log_mutex_wait(void* lock) {
#ifdef CONFIG_EVENT_MUTEX_WAIT
  init_event(struct mutex_wait_event, EVENT_MUTEX_WAIT, event);
  event->lock = (__le32) lock;
  finish_event();
#endif
}

static inline void event_log_mutex_wake(void* lock) {
#ifdef CONFIG_EVENT_MUTEX_WAKE
  init_event(struct mutex_wake_event, EVENT_MUTEX_WAKE, event);
  event->lock = (__le32) lock;
  finish_event();
#endif
}

static inline void event_log_mutex_notify(void* lock, pid_t pid) {
#ifdef CONFIG_EVENT_MUTEX_NOTIFY
  init_event(struct mutex_notify_event, EVENT_MUTEX_NOTIFY, event);
  event->lock = (__le32) lock;
  event->pid = pid;
  finish_event();
#endif
}

static inline void event_log_sem_lock(void* lock) {
#ifdef CONFIG_EVENT_SEMAPHORE_LOCK
  init_event(struct sem_lock_event, EVENT_SEMAPHORE_LOCK, event);
  event->lock = (__le32) lock;
  finish_event();
#endif
}

static inline void event_log_sem_wait(void* lock) {
#ifdef CONFIG_EVENT_SEMAPHORE_WAIT
  init_event(struct sem_wait_event, EVENT_SEMAPHORE_WAIT, event);
  event->lock = (__le32) lock;
  finish_event();
#endif
}

#endif // __KERNEL__
#endif // EVENTLOGGING_EVENTS_H
