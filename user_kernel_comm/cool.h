#ifndef COOL_H_
#define COOL_H_
 
/* This header is shared with the user-mode application. We must have the same communication interface. */
 
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
 
typedef unsigned int uint;
#endif
 
/* To get this value, just increase the last value found in the
 * defines: https://elixir.bootlin.com/linux/v5.8.16/source/include/uapi/linux/netlink.h#L9
 * Just make sure it's not over MAX_LINKS. */
#define NETLINK_COOL 23
 
enum {
        MSG_TYPE_STRING = NLMSG_MIN_TYPE, /* Anything below this value it's a control message, and won't be passed to us in kernel. */
        MSG_TYPE_ERROR,
        /* ... */
 
        __MSG_TYPE_MAX
};
 

struct shared_msg_err {
        int number;
        bool critical;
};
 
#endif // COOL_H_