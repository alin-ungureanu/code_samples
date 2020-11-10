#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>

#include "cool.h"
 
/* A good practice to have all of these */
MODULE_DESCRIPTION("A cool little kernel driver");
MODULE_AUTHOR("aungureanu@riverbed.com");
MODULE_LICENSE("GPL");


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


static unsigned int cool_dump_packet(void *priv, struct sk_buff *skb,
                                     const struct nf_hook_state *state, bool in)
{
        /* ....... */
        if (in)
        	pr_info("in packet\n");
        else
        	pr_info("out packet\n");
 
        return NF_ACCEPT; // see https://elixir.bootlin.com/linux/v5.8.16/source/include/uapi/linux/netfilter.h#L10 for other values
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
	int err;
        pr_info("cool module is initializing!\n");
 
        err = nf_register_net_hooks(&init_net, netfilter_ops, ARRAY_SIZE(netfilter_ops));
        if (err)
                pr_err("nf_register_net_hook failed: %d\n", err);
        else
                pr_info("cool: registerd netfilter hooks\n");
 
        return err;
}
 
static void cool_uninit(void)
{
        pr_info("cool module is uninitializing!\n");
        nf_unregister_net_hooks(&init_net, netfilter_ops, ARRAY_SIZE(netfilter_ops));
}
 
 
/* This is actually the part which marks the init/uninit functions. */
module_init(cool_init);
module_exit(cool_uninit);
