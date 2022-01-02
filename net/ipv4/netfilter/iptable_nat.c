// SPDX-License-Identifier: GPL-2.0-only
/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2011 Patrick McHardy <kaber@trash.net>
 */

#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/ip.h>
#include <net/ip.h>

#include <net/netfilter/nf_nat.h>

static int __net_init iptable_nat_table_init(struct net *net);

static const struct xt_table nf_nat_ipv4_table = {
	.name		= "nat",
	.valid_hooks	= (1 << NF_INET_PRE_ROUTING) |
			  (1 << NF_INET_POST_ROUTING) |
			  (1 << NF_INET_LOCAL_OUT) |
			  (1 << NF_INET_LOCAL_IN),
	.me		= THIS_MODULE,
	.af		= NFPROTO_IPV4,
	.table_init	= iptable_nat_table_init,
};

static unsigned int iptable_nat_do_chain(void *priv,
					 struct sk_buff *skb,
					 const struct nf_hook_state *state)
{
	return ipt_do_table(skb, state, state->net->ipv4.nat_table);
}

/**
 * @brief 
 * iptables 实现源码
 * struct nf_hook_ops {
 *	nf_hookfn		*hook; // 报文处理函数
 *	struct net_device	*dev;
 *	void			*priv;
 *	u_int8_t		pf; // 协议簇
 *	unsigned int		hooknum; // 指定在哪个 hook 点位上
 *	int			priority; // 优先级, 越小的优先级越高(enum nf_ip_hook_priorities 以及 enum nf_ip6_hook_priorities {)
 * }; 
 * 
 * 
 * 
 * 这里现在是 ipv4 相关
 * 在 ipv6 下面也有 ipv6 相关的几张表的钩子注册 
 * 这里是 nat 表的初始化
 * 对于 nat 来说不同点位的优先级不一样, 虽然执行函数都一样吧
 * 
 * 对于 filter 或其他表来说, 可能优先级和执行的函数都一样
 * 然后就直接用 xt_hook_ops_alloc(packet_filter, hook 函数) 方法做函数注册了
 * 
 * 首先每个表都有个私有的 static const struct xt_table packet_filter 结构
 * packet_filter 上定义了优先级, 协议簇, 还有有效的钩子点位
 * 然后 xt_hook_ops_alloc 方法就遍历这些钩子点位
 * 最后做出一条 nf_hook_ops 的数组
 * 数组中每一项除了点位不同, 剩下的 hook 函数, 协议, 还有优先级都相同
 * 
 * 其实最后 xt_hook_ops_alloc 处理完得到的就是类似 nat 中的 nf_nat_ipv4_ops[]
 * 所以不管是 nat 还是 filter 之类的, 都是有这么一个 ops[]
 * 然后在初始化时执行的 ipt_register_table 方法中把这个数组传进去
 * 
 * 每当创建了一个新的 net ns 时, 都会执行一次 ipt_register_table
 * 并且把当前的 netns 作为参数传进去
 * 
 * ipt_register_table(
 * 	net, // 当前的命名空间
 * 	&packet_filter, // 每个表的基础信息(包括名字, 协议簇类型等)
 * 	repl,
 * 	filter_ops, // filter 表的类似 nf_nat_ipv4_ops 的东西, 可能是 null
 * 	&net->ipv4.iptable_filter // 命名空间下的不同协议的表项指针, 有 ipv4 和 ipv6
 * );
 * 
 * 在这个函数中, 就会把 filter_ops 中的每一项给绑到对应的 netns 下的不同协议的不同钩子点位上
 * 在 nat 中的 ipt_nat_register_lookups 方法也有类似的作用
 * 比如在 __nf_register_net_hook 函数中, 会先执行一个:
 * 	pp = nf_hook_entry_head(net, pf, reg->hooknum, reg->dev);
 * 	其中 net 是命名空间, pf 是协议簇, hooknum 表示钩子点位, dev 是网卡设备(不过里面并没用上)
 * 	该方法作用就是找到该 netns 结构体下的 xx 协议中的 yy 钩子点位的地址
 * 接下来执行 nf_hook_entries_grow 方法, 就是把新旧的 hook 按照优先级做个 merge
 * 之后交给 pp
 * 
 * 也就是说, 就相当于:
 * 	netns = {
 * 		ipv4: {
 * 			nat: [hook1, hook2, ......],
 * 			filter: [hook1, hook2, ......],
 * 			......
 * 		},
 * 		ipv6: {
 * 			
 * 		}
 * 	}
 * 
 * 然后执行的时候, 通过 NF_HOOK 执行
 * NF_HOOK(aaa, bbb, net, sk, skb, indev, outdev, okfn);
 * 
 * 一共传进去 8 个参数
 * 里面做的事情就是说:
 * 	找到在 net 的 aaa 协议的 bbb 点位
 * 	然后用 sk, indev, outdev, okfn 做出一个 state 结构体:
 * 	{
 *		p->hook = hook;
 *		p->pf = pf;
 *		p->in = indev;
 *		p->out = outdev;
 *		p->sk = sk;
 *		p->net = net;
 *		p->okfn = okfn;
 *	}
 *
 * 之后遍历上面找到的点位, 分别执行点位上的每个 hook 函数
 * 并用 skb 以及上面得到的 state 作为参数(还有个 priv 私有指针参数, 不知道干啥的)
 * 
 * 
 * 
 * 
 * 
 * iptables 的注册是在一开始内核执行的时候就注册上的
 * module_init(iptable_filter_init);
 * module_init(iptable_mangle_init);
 * module_init(iptable_raw_init);
 * module_init(iptable_security_init);
 * module_init(iptable_nat_init);
 * iptable_nat_init
 * 	-> iptable_nat_table_init (他也是五张表都有对应的函数实现)
 * 		-> ipt_nat_register_lookups
 * 		它里面有个 for 循环, 循环里头就是遍历 nf_nat_ipv4_ops
 * 		然后每次都把 nf_nat_ipv4_ops 中的成员作为参数给下面的函数
 * 			-> nf_nat_ipv4_register_fn
 * 				-> nf_nat_register_fn
 * 					-> nf_register_net_hooks
 * 					这里面循环 nf_nat_ipv4_ops[] 这个数组
 * 						-> nf_register_net_hook
 * 							-> __nf_register_net_hook
 * 								-> nf_hook_entries_grow
 * 								这里头创建一个 nf_hook_entries 类型的 new 结构
 * 								然后去找旧的 nf_hook_entries 类型
 * 								依次遍历旧的 nf_hook_entries
 * 								对比本次执行 nf_hook_entries_grow 时传进来的 nf_hook_ops 对象
 * 								看 nf_hook_ops 对象上的优先级, 和旧的每一项对比
 * 								然后按照优先级的顺序把旧的和本次新进来的做成一条链表干给新生成的 nf_hook_entries
 * 								nf_hook_entries 数据结构上有个 hooks 数组
 * 								数组每一项都是 nf_hook_entry 类型
 * 								nf_hook_entry 上有个 hook 和 priv, 其中 hook 就是钩子函数
 * 
 * 
 * 每次要执行对应的钩子函数的时候
 * 会执行:
 * 	NF_HOOK (NF_HOOK(uint8_t pf, ...))
 * 		-> nf_hook (static inline int nf_hook(u_int8_t pf, ...))
 * 		在这个函数中会根据协议类型决定要查找哪条链表
 * 			-> nf_hook_slow
 * 			这里头会遍历整个点位上的链表的 hook 函数
 * 			-> nf_hook_entry_hookfn
 * 			这里头执行钩子函数
 * 			会把:
 * 				entry->priv 私有指针, 一般不会指定
 * 				skb 报文缓冲区
 * 				nf_hook_state 报文收发状态信息, 比如从哪儿来, 要到哪儿去之类的信息
 * 			等三个东西作为参数传进来
 * 			之后会根据钩子函数的返回值来决定下一步
 * 			比如如果是 NF_ACCEPT 就直接 break, 进入到下一个协议栈
 * 			NF_DROP 就把 skb 丢弃
 * 			NF_QUEUE 就 continue
 * 
 */
static const struct nf_hook_ops nf_nat_ipv4_ops[] = {
	{
		.hook		= iptable_nat_do_chain,
		.pf		= NFPROTO_IPV4,
		.hooknum	= NF_INET_PRE_ROUTING,
		.priority	= NF_IP_PRI_NAT_DST,
	},
	{
		.hook		= iptable_nat_do_chain,
		.pf		= NFPROTO_IPV4,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_NAT_SRC,
	},
	{
		.hook		= iptable_nat_do_chain,
		.pf		= NFPROTO_IPV4,
		.hooknum	= NF_INET_LOCAL_OUT,
		.priority	= NF_IP_PRI_NAT_DST,
	},
	{
		.hook		= iptable_nat_do_chain,
		.pf		= NFPROTO_IPV4,
		.hooknum	= NF_INET_LOCAL_IN,
		.priority	= NF_IP_PRI_NAT_SRC,
	},
};

static int ipt_nat_register_lookups(struct net *net)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(nf_nat_ipv4_ops); i++) {
		ret = nf_nat_ipv4_register_fn(net, &nf_nat_ipv4_ops[i]);
		if (ret) {
			while (i)
				nf_nat_ipv4_unregister_fn(net, &nf_nat_ipv4_ops[--i]);

			return ret;
		}
	}

	return 0;
}

static void ipt_nat_unregister_lookups(struct net *net)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(nf_nat_ipv4_ops); i++)
		nf_nat_ipv4_unregister_fn(net, &nf_nat_ipv4_ops[i]);
}

static int __net_init iptable_nat_table_init(struct net *net)
{
	struct ipt_replace *repl;
	int ret;

	if (net->ipv4.nat_table)
		return 0;

	repl = ipt_alloc_initial_table(&nf_nat_ipv4_table);
	if (repl == NULL)
		return -ENOMEM;
	ret = ipt_register_table(net, &nf_nat_ipv4_table, repl,
				 NULL, &net->ipv4.nat_table);
	if (ret < 0) {
		kfree(repl);
		return ret;
	}

	ret = ipt_nat_register_lookups(net);
	if (ret < 0) {
		ipt_unregister_table(net, net->ipv4.nat_table, NULL);
		net->ipv4.nat_table = NULL;
	}

	kfree(repl);
	return ret;
}

static void __net_exit iptable_nat_net_exit(struct net *net)
{
	if (!net->ipv4.nat_table)
		return;
	ipt_nat_unregister_lookups(net);
	ipt_unregister_table(net, net->ipv4.nat_table, NULL);
	net->ipv4.nat_table = NULL;
}

static struct pernet_operations iptable_nat_net_ops = {
	.exit	= iptable_nat_net_exit,
};

static int __init iptable_nat_init(void)
{
	int ret = register_pernet_subsys(&iptable_nat_net_ops);

	if (ret)
		return ret;

	ret = iptable_nat_table_init(&init_net);
	if (ret)
		unregister_pernet_subsys(&iptable_nat_net_ops);
	return ret;
}

static void __exit iptable_nat_exit(void)
{
	unregister_pernet_subsys(&iptable_nat_net_ops);
}

module_init(iptable_nat_init);
module_exit(iptable_nat_exit);

MODULE_LICENSE("GPL");
