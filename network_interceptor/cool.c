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



/* Defines whether the nf hooks are enabled or not */
static bool cool_nf_enabled = false;
/* We need this global, so we can remove it at exit */
struct proc_dir_entry *cool_proc_dir, *proc_enabled, *proc_ip_dest;

unsigned int dest_ip = 0x7F000001; //127.0.0.1
unsigned int dest_port = 0x0000;
 
static const struct file_operations cool_proc_ops = {
        .open = cool_proc_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = single_release,
        .write = cool_proc_write,
};

static const struct file_operations cool_proc_ip_dest_ops = {
        .open = cool_proc_ip_dest_open,
        .read = seq_read,
        .llseek = seq_lseek,
        .release = single_release,
        .write = cool_proc_ip_dest_write,
};


/* must be declared globally since netfilter will not create a copy of it */
static struct nf_hook_ops netfilter_ops[] = {
        {
                .hook = cool_in_hookfn,
                .pf = NFPROTO_INET,
                .hooknum = NF_INET_PRE_ROUTING,
                .priority = NF_IP_PRI_FIRST
        },

        {
                .hook = cool_out_hookfn,
                .pf = NFPROTO_INET,
                .hooknum = NF_INET_LOCAL_OUT,
                .priority = NF_IP_PRI_FIRST
        },
};

static int cool_proc_show(struct seq_file *m, void *v)
{
        seq_printf(m, "%d\n", cool_nf_enabled);
        return 0;
}

static int cool_proc_ip_dest_show(struct seq_file *m, void *v)
{
        seq_printf(m, "%d.%d.%d.%d:%d\n", (unsigned char)(dest_ip >> 24),
                                        (unsigned char)(dest_ip >> 16),
                                        (unsigned char)(dest_ip >> 8),
                                        (unsigned char)(dest_ip),
                                         dest_port);
        return 0;
}

 
static int cool_proc_open(struct inode *inode, struct  file *file)
{
        return single_open(file, cool_proc_show, NULL);
}

static int cool_proc_ip_dest_open(struct inode *inode, struct  file *file)
{
        return single_open(file, cool_proc_ip_dest_show, NULL);
}



static ssize_t cool_proc_ip_dest_write(struct file *file,
                               const char __user *buffer, size_t count, loff_t *pos)
{
        char buf[32] = { 0 };
        
        int err;
        unsigned int ipH, iph, ipl, ipL;
        
        size_t size = min(sizeof(buf), count);
 
        if ((err = copy_from_user(buf, buffer, size)))
                return -err;
 
        buf[size] = '\0';
        sscanf(buf, "%d.%d.%d.%d:%d", &ipH, &iph, &ipl, &ipL, &dest_port);
        dest_ip = (ipH << 24) + (iph << 16) + (ipl << 8) + ipL;
        
        return count;
}


static ssize_t cool_proc_write(struct file *file,
                               const char __user *buffer, size_t count, loff_t *pos)
{
        char buf[16] = { 0 };
        int err;
        bool enabled;
        size_t size = min(sizeof(buf), count);
 
        if ((err = copy_from_user(buf, buffer, size)))
                return -err;
 
        /* See the documentation on elixir */
        if ((err = kstrtobool(buf, &enabled)))
                return -err;
        if (!cool_nf_enabled)
        {
                /* Now do the  nf_register_net_hooks/nf_unregister_net_hooks here. */
                err = nf_register_net_hooks(&init_net, netfilter_ops, ARRAY_SIZE(netfilter_ops));
                if (err)
                        pr_err("nf_register_net_hook failed: %d\n", err);
                else
                        pr_info("cool: registerd netfilter hooks\n");
        
                /* Don't forget to update the global state in cool_nf_enabled */
                cool_nf_enabled = true;
        }
        else
        {
                nf_unregister_net_hooks(&init_net, netfilter_ops, ARRAY_SIZE(netfilter_ops));
                cool_nf_enabled = false;
        }
        /* We'll blindly tell that we consumed the whole buffer. */
        return count;
}


static unsigned int cool_dump_packet(void *priv, struct sk_buff *skb,
                                     const struct nf_hook_state *state, bool in)
{
 	__be32 src_addr, dst_addr;
 	unsigned int src_ip, dst_ip;
 	unsigned short dst_port, src_port;
 	
 	struct tcphdr * tcp_header;
 	struct udphdr * udp_header;
 	struct iphdr * ip_header = (struct iphdr *)ip_hdr(skb);
 	src_addr = ip_header->saddr;
 	dst_addr = ip_header->daddr;
 	src_ip = __be32_to_cpu(src_addr);
 	dst_ip = __be32_to_cpu(dst_addr);
 	if (ip_header->protocol == IPPROTO_TCP)
 	{
 		tcp_header = (struct tcphdr *)((__u32 *)ip_header + ip_header->ihl);
 		src_port = ntohs((unsigned short int) tcp_header->source);
		dst_port = ntohs((unsigned short int) tcp_header->dest);
 	}
 	else
 	{
 		udp_header = (struct udphdr *)((__u32 *)ip_header + ip_header->ihl);
 		src_port = ntohs((unsigned short int) udp_header->source);
		dst_port = ntohs((unsigned short int) udp_header->dest);
 	}
 	
 	if (dst_port != dest_port || dst_ip != dest_ip)
        	return NF_ACCEPT; // see elixir for other values
        else
        	return NF_DROP;
}
 
static unsigned int cool_in_hookfn(void *priv, struct sk_buff *skb,
                                   const struct nf_hook_state *state)
{
        return cool_dump_packet(priv, skb, state, true);
}
 
static unsigned int cool_out_hookfn(void *priv, struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
        return cool_dump_packet(priv, skb, state, false);
}

/* This will be the init function. The guide is to have it as `modulename`_init,
 * but it can be anything. Please notice the `(void)` instead of `()`. */
static int cool_init(void)
{
        pr_info("cool module is initializing!\n");
        cool_proc_dir = proc_mkdir("cool", NULL);
        if (!cool_proc_dir) {
                pr_err("proc_mkdir failed to register /proc/cool/\n");
                return ENOMEM;
        }

	proc_enabled = proc_create("enabled", 0666, cool_proc_dir, &cool_proc_ops);
        if (!proc_enabled) {
                pr_err("proc_create failed to register /proc/cool/enabled\n");
                return ENOMEM;
        }
        proc_ip_dest = proc_create("destination", 0666, cool_proc_dir, &cool_proc_ip_dest_ops);
        if (!proc_enabled) {
                pr_err("proc_create failed to register /proc/cool/destination\n");
                return ENOMEM;
        }
 
        return 0;
}
 
static void cool_uninit(void)
{
        pr_info("cool module is uninitializing!\n");
        if (cool_nf_enabled) {
                pr_info("cool: unregistering net hooks...");
                nf_unregister_net_hooks(&init_net, netfilter_ops, ARRAY_SIZE(netfilter_ops));
        }
 
        if (proc_enabled)
                proc_remove(proc_enabled);
        if (cool_proc_dir)
                proc_remove(cool_proc_dir);
        if (proc_ip_dest)
                proc_remove(proc_ip_dest);
        
}
 
 
/* This is actually the part which marks the init/uninit functions. */
module_init(cool_init);
module_exit(cool_uninit);
