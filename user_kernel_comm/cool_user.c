#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
 
#include "cool.h"
 
static int comm_sk;
static struct iovec iov;
static struct msghdr msg;

#define STR_TO_SEND "Hello there!"

void send_kernel_msg(void)
{

        /* We need NLMSG_SPACE to calculate how much space we must allocate */
        const size_t nlmsg_len = NLMSG_SPACE(sizeof(STR_TO_SEND));
 
        struct nlmsghdr *nlh = calloc(1, nlmsg_len);
        nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
        nlh->nlmsg_len = nlmsg_len;
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_type = MSG_TYPE_STRING;
 
        /* This is our actuall data which we will fill */
        strcpy(NLMSG_DATA(nlh), STR_TO_SEND);
 
        /* You could have an array of iov structures, and send all of those at once.
         * struct iovec is used for any data send to the kernel by other interfacess too (eg: process_vm_readv). */
        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;
 
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
 
        /* MSG_DONTWAIT if you want non-blocking, but that implies handling EAGAIN and EWOULDBLOCK */
        if (sendmsg(comm_sk, &msg, MSG_CONFIRM) < 0)
                perror("sendmsg failed");
 
        free(nlh);
}

void recv_kernel_msg(void)
{
        int len;
        char buf[4096];
        struct sockaddr_nl sa;
 
        struct iovec iov = {
                .iov_base = buf,
                .iov_len = sizeof(buf),
        };
 
        struct msghdr msg = {
                .msg_name = &sa,
                .msg_namelen = sizeof(sa),
                .msg_iov = &iov,
                .msg_iovlen = 1,
                /* rest are optional, leave them zeroed */
        };
 
        /* MSG_DONTWAIT if you want non-blocking, but that implies handling EAGAIN and EWOULDBLOCK */
        len = recvmsg(comm_sk, &msg, 0);
 
        /* for_each_nlmsghdr */
        for (struct nlmsghdr *nlh = (struct nlmsghdr *)buf; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len))
        {
                if (nlh->nlmsg_type == NLMSG_ERROR) {
                        fprintf(stderr, "got an error message");
                        continue;
                }
 
                /* NOTE: Assumes you get back a string. */
                printf("Got back type %d, reply: %s\n", nlh->nlmsg_type, (char *)NLMSG_DATA(nlh));
 
                if (nlh->nlmsg_type == NLMSG_DONE)
                        break;
        }
}
 
int main(int argc, char **argv)
{
        comm_sk = socket(PF_NETLINK, SOCK_RAW, NETLINK_COOL);
        if (comm_sk <= 0) {
                perror("socket failed");
                return 1;
        }
 
        struct sockaddr_nl src_addr = {
                .nl_family = AF_NETLINK,
                .nl_pid = getpid()
                /* .nl_groups -> If you use the groups field in the netlink_kernel_cfg structure */
        };
 
        if (bind(comm_sk, (struct sockaddr *)&src_addr, sizeof(src_addr))) {
                perror("bind failed");
                return 1;
        }
 
        send_kernel_msg();

        recv_kernel_msg();
 
        /* ... cleanup ... */
 
        return 0;
}