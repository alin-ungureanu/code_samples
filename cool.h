#ifndef COOL_H_
#define COOL_H_
 
/* This header is shared with the user-mode application. We must have the same communication interface. */
 
//#ifdef __KERNEL__
#include <linux/types.h>

//#else
#if 0
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
 
typedef unsigned int uint;
#endif

static int cool_proc_open(struct inode *inode, struct  file *file);

static ssize_t cool_proc_write(struct file *file,
                               const char __user *buffer, size_t count, loff_t *pos);

static unsigned int cool_in_hookfn(void *priv, struct sk_buff *skb,
                                   const struct nf_hook_state *state);



static unsigned int cool_out_hookfn(void *priv, struct sk_buff *skb,
                                    const struct nf_hook_state *state);










#endif // COOL_H_
