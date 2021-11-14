// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2011 Instituto Nokia de Tecnologia
 *
 * Authors:
 *    Aloisio Almeida Jr <aloisio.almeida@openbossa.org>
 *    Lauro Ramos Venancio <lauro.venancio@openbossa.org>
 */

#include <linux/socket.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/uio.h>

#include <net/sock.h>
#define TEST_AFSOCK_DEBUG
static char pkt_buf[2048] = {0};
static int pkt_len = 0;
static void test_proto_unhash(struct sock *sk)
{
	printk("<%s:%d>\n",__FUNCTION__, __LINE__);
	return;
}

static int test_proto_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int noblock, int flags, int *addr_len)
{
	int n = 0;
	printk("<%s:%d>\n",__FUNCTION__, __LINE__);
	if(!pkt_len)
		return 0;
	
	n = copy_to_iter(pkt_buf, pkt_len, &msg->msg_iter);
	if (n != pkt_len)
		return -EFAULT;
#ifdef TEST_AFSOCK_DEBUG
	print_hex_dump(KERN_ERR, "recv packet: ", DUMP_PREFIX_OFFSET,
		 16, 1, pkt_buf, pkt_len, true);
#endif
	pkt_len = 0;
	return n;
}

static int test_proto_sendmsg(struct sock *sk, struct msghdr *msg, size_t len)
{

	printk("<%s:%d>\n",__FUNCTION__, __LINE__);
	if (!copy_from_iter_full(pkt_buf, len, &msg->msg_iter))
		return -EFAULT;
	pkt_len = len;
#ifdef TEST_AFSOCK_DEBUG
	print_hex_dump(KERN_ERR, "send packet: ", DUMP_PREFIX_OFFSET,
		 16, 1, pkt_buf, pkt_len, true);
#endif

	return 0;
}

static struct proto test_proto = {
	.name	  = "TEST",
	.owner	  = THIS_MODULE,
	.unhash	  = test_proto_unhash,
	.sendmsg		= test_proto_sendmsg,
	.recvmsg		= test_proto_recvmsg,
	.obj_size = sizeof(struct sock),
};

static int test_afsock_release(struct socket *sock)
{
		struct sock *sk = sock->sk;
		if(sk)
			sk_common_release(sk);
		return 0;
}

int	test_afsock_sendmsg(struct socket *sock, struct msghdr *m, size_t total_len)
{
	struct sock *sk = sock->sk;

	printk("<%s:%d>\n",__FUNCTION__, __LINE__);
	sk->sk_prot->sendmsg(sk, m, total_len);

	return 0;
}

int test_afsock_recvmsg(struct socket *sock, struct msghdr *m, size_t total_len, int flags)
{
	
	struct sock *sk = sock->sk;
	printk("<%s:%d>\n",__FUNCTION__, __LINE__);
	
	sk->sk_prot->recvmsg(sk, m, total_len, 1, 0 ,0);
	return 0;
}

static const struct proto_ops test_afsock_pf_ops = {
	.family 	   = PF_DINGTEST,
	.owner		   = THIS_MODULE,
	.release	   = test_afsock_release,
	.sendmsg	   = test_afsock_sendmsg,
	.recvmsg	   = test_afsock_recvmsg,
};

static void test_afsock_destruct(struct sock *sk)
{
	skb_queue_purge(&sk->sk_error_queue);
	skb_queue_purge(&sk->sk_receive_queue);
	dst_release(sk->sk_dst_cache);
	dst_release(sk->sk_rx_dst);

}

static int test_afsock_create(struct net *net, struct socket *sock, int proto, int kern)
{
	struct sock *sk = NULL;
	
	sock->ops = &test_afsock_pf_ops;
	sk = sk_alloc(net, PF_DINGTEST, GFP_KERNEL, &test_proto, kern);
	if (!sk)
		return -ENOMEM;
	
	sock_init_data(sock, sk);
	sk->sk_destruct = test_afsock_destruct;
	sk->sk_protocol = proto;
	sk->sk_family 	= PF_DINGTEST;
	sk->sk_backlog_rcv = sk->sk_prot->backlog_rcv;
	
	return 0;
}

static const struct net_proto_family test_afsock_family_ops = {
	.owner  = THIS_MODULE,
	.family = PF_DINGTEST,
	.create = test_afsock_create,
};
	

int __init test_afsock_init(void)
	
{	
	int rc = 0;
	rc = proto_register(&test_proto, 1);
	if(rc)
		goto proto_failed;
	
	rc = sock_register(&test_afsock_family_ops);
	if(rc)
		goto sock_failed;
	
	return 0;
	
sock_failed:
	sock_unregister(PF_DINGTEST);
proto_failed:
	proto_unregister(&test_proto);
	return rc;
}

void __exit test_afsock_exit(void)
{
	proto_unregister(&test_proto);
	sock_unregister(PF_DINGTEST);
}
module_init(test_afsock_init);
module_exit(test_afsock_exit);

MODULE_AUTHOR("Kees Cook <keescook@chromium.org>");
MODULE_LICENSE("GPL");

