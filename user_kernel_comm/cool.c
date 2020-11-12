#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/uaccess.h>
//#include <linux/string.h>
//#include <linux/inet.h>

#include "cool.h"
 
/* A good practice to have all of these */
MODULE_DESCRIPTION("A cool little kernel driver");
MODULE_AUTHOR("aungureanu@riverbed.com");
MODULE_LICENSE("GPL");

#define STR_TO_SEND "Hello from kernel"

struct sock *comm_sk = NULL;



static void cool_send_reply(int pid)
{

        const int msg_size = sizeof(STR_TO_SEND);
        struct nlmsghdr *nlh;
 
        struct sk_buff *skb = nlmsg_new(msg_size, 0);
        if (!skb) {
                pr_err("nlmsg_new failed\n");
                return;
        }
 
        nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, msg_size, 0);
        strncpy(nlmsg_data(nlh), STR_TO_SEND, msg_size);
 
        if (nlmsg_unicast(comm_sk, skb, pid) < 0)
                pr_err("nlmsg_unicast failed\n");
}

static int cool_recv_netlink(struct sk_buff *skb, struct nlmsghdr *nlh,
                             struct netlink_ext_ack *extack)
{
        /* We don't have anything else to do, we get the nlmsghdr directly. We use `nlmsg_type` for our type, and NLMSG_DATA(nlh) for data. */
 
        if (nlh->nlmsg_type >= __MSG_TYPE_MAX) {
                pr_err("cool: invalid message type: %d\n", nlh->nlmsg_type);
                return -EOPNOTSUPP;
        }
 
        switch (nlh->nlmsg_type) {
        case MSG_TYPE_STRING:
                /* Note that we don't need copy_from_user */
                pr_info("Got from usermode (len %d): %s\n", nlh->nlmsg_len, (char *)NLMSG_DATA(nlh));
                break;
        case MSG_TYPE_ERROR: {
                struct shared_msg_err *err =  NLMSG_DATA(nlh);
                pr_info("Got an error (len %d): %d, critical %d\n", nlh->nlmsg_len, err->number, err->critical);
                break;
        }
        default:
                pr_info("Unhandled message type: %d\n", nlh->nlmsg_len);
                break;
        }
 
        cool_send_reply(nlh->nlmsg_pid);
 
        return 0;
}

static void cool_recv(struct sk_buff *skb)
{
        /* netlink_rcv_skb knows when to call our callback in case of multi-part, ack, etc.
         * See the implementation for more details. */
        netlink_rcv_skb(skb, &cool_recv_netlink);
}

static int cool_init(void)
{
        struct netlink_kernel_cfg cfg = {
                .input = cool_recv
        };
 
        pr_info("cool: module is initializing!\n");
 
        comm_sk = netlink_kernel_create(&init_net, NETLINK_COOL, &cfg);
        if (!comm_sk) {
                pr_err("cool: failed to register netlink\n");
                return -ENOMEM;
        }
 
        return 0;
}
 
static void cool_uninit(void)
{
        pr_info("cool module is uninitializing!\n");
        if (comm_sk)
                netlink_kernel_release(comm_sk);
        
}
 
 
/* This is actually the part which marks the init/uninit functions. */
module_init(cool_init);
module_exit(cool_uninit);
