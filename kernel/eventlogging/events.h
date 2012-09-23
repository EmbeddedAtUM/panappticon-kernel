#include <linux/types.h>
#include <linux/kernel.h>

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
  u8  event_type;
  __le32 timestamp_s;
  __le32 timestamp_us;
  u8     proc;
  __le16 pid;
};

struct context_switch_event {
  struct event_hdr hdr;
  __le16 old_pid;
  __le16 new_pid;
};

struct network_block_event {
  struct event_hdr hdr;
};
