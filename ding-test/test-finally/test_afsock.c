// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/socket.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/uio.h>

#include <net/sock.h>
static char pkt_buf[2048] = {0};
static int pkt_len = 0;

static int ding_test_proto_recvmsg(struct sock *sk, struct msghdr *msg, size_t len, int noblock, int flags, int *addr_len) {
	int n = 0;
	if(!pkt_len) return 0;
	
	// 通过 copy_to_iter 将自定义的 pkt_buf 这个 buffer 中的数据从内核空间拷贝到数据空间
	// 简单点说就是将 pkt_len 这么长字节数的 pkt_buf 中的数据, 拷贝到 msg_iter 这个迭代器指向的用户空间地址
	n = copy_to_iter(pkt_buf, pkt_len, &msg->msg_iter);
	printk("内核接收: %s", pkt_buf);
	if (n != pkt_len) return -EFAULT;
	pkt_len = 0;
	return n;
}

// 在套接字发送接收系统调用流程中，send/recv，sendto/recvfrom，sendmsg/recvmsg最终都会使用内核中的msghdr来组织数据
// msg_iter 是 iov_iter 这样的一个结构体, 里面的信息就可以描述数据
static int ding_test_proto_sendmsg(struct sock *sk, struct msghdr *msg, size_t len) {
	// copy_from_iter_full 用来把 msg_iter 报文内容拷贝到自定义的 buffer 中
	if (!copy_from_iter_full(pkt_buf, len, &msg->msg_iter)) return -EFAULT;
	pkt_len = len;
	printk("内核发送: %s", pkt_buf);
	return 0;
}

static void ding_test_proto_unhash(struct sock *sk) { printk("这里是 ding_test_proto_unhash 方法"); return; }
static int ding_test_afsock_bind(struct socket *sock, struct sockaddr *myaddr, int sockaddr_len) { printk("这里是 ding_test_afsock_bind 方法"); return 0; }

static int ding_test_afsock_release(struct socket *sock) {
	struct sock *sk = sock->sk;
	// sk_common_release 是内核提供用来析构的 socket 的方法
	if(sk) sk_common_release(sk);
	return 0;
}

int	ding_afsock_sendmsg(struct socket *sock, struct msghdr *m, size_t total_len) {
	struct sock *sk = sock->sk;
	printk("内核中 ding socket 发送消息");
	// 这里调用指定的四层协议栈的 sendmsg 函数
	// 也就是下面 ding_test_proto 中的 sendmsg
	sk->sk_prot->sendmsg(sk, m, total_len);
	return 0;
}

int ding_test_afsock_recvmsg(struct socket *sock, struct msghdr *m, size_t total_len, int flags) {
	struct sock *sk = sock->sk;
	// printk("内核中 ding socket 接收消息");
	// 这里调用指定的四层协议栈指定的 recvmsg 函数
	// 也就是下面 ding_test_proto 中的 recvmsg
	sk->sk_prot->recvmsg(sk, m, total_len, 1, 0 ,0);
	return 0;
}

// 这些操作集就是自定义协议簇的一些接口
static const struct proto_ops test_afsock_pf_ops = {
	.family 	   = PF_DINGTEST,
	.owner		   = THIS_MODULE,
	.bind		   = ding_test_afsock_bind,
	.setsockopt    = sock_common_setsockopt,
	.getsockopt    = sock_common_getsockopt,
	// 下边这三个是最主要的
	.release	   = ding_test_afsock_release, // 关闭套接字时调用的
	.sendmsg	   = ding_afsock_sendmsg, // 使用套接字发送时用
	.recvmsg	   = ding_test_afsock_recvmsg, // 使用套接字接收时用
};

// 这里使用自定义的四层协议栈类型, 也可以直接使用 tcp 或者 udp 以及 ICMP 之类的
static struct proto ding_test_proto = {
	.name	  = "DING_TEST",
	.owner	  = THIS_MODULE,
	// 必须实现的方法有如下三个
	.unhash	  = ding_test_proto_unhash, // 释放 socket 时调用
	// 下面俩函数中要进行具体的传输层协议栈的收发包实现
	.sendmsg		= ding_test_proto_sendmsg, // 发包时调用
	.recvmsg		= ding_test_proto_recvmsg, // 收包时调用
	.obj_size = sizeof(struct sock),
};

// net 是网络命令空间 task_struct -> nsproxy -> net_ns
static int ding_test_afsock_create(struct net *net, struct socket *sock, int proto, int kern) {
	struct sock *sk = NULL;
	
	// 将自定义协议簇的操作集赋给 socket 结构体
	sock->ops = &test_afsock_pf_ops;
	// 创建要使用具体协议栈中操作集的 sock 结构体的实例
	// 创建的时候需要指定使用自定义的协议簇
	// 以及指定 ding_test_proto 也就是四层协议栈的类型
	sk = sk_alloc(net, PF_DINGTEST, GFP_KERNEL, &ding_test_proto, kern);
	if (!sk) return -ENOMEM;
	
	// 初始化 sock 和 sk 中的一些东西, 并让它俩能互相引用
	// 往下基本上可以说是一些固定的流程
	sock_init_data(sock, sk);
	sk->sk_protocol = proto;
	sk->sk_family 	= PF_DINGTEST;
	// 接收队列已经满了的时候会暂时先将报文放到 backlog_rcv 队列中
	sk->sk_backlog_rcv = sk->sk_prot->backlog_rcv;
	
	return 0;
}

static const struct net_proto_family ding_test_afsock_family_ops = {
	.owner  = THIS_MODULE,
	.family = PF_DINGTEST, // 这里是 include/linux/socket.h 中自己加的编号
	.create = ding_test_afsock_create, // 执行 socket 系统调用创建一个 socket 时候就会调用这个 create
};

int __init ding_test_afsock_init(void) {
	int rc = 0;
	// 注册自己实现的协议簇
	rc = sock_register(&ding_test_afsock_family_ops);
	if(rc) goto sock_failed;
	return 0;
	
sock_failed:
	sock_unregister(PF_DINGTEST);
	return rc;
}

void __exit ding_test_afsock_exit(void) {
	sock_unregister(PF_DINGTEST);
}
module_init(ding_test_afsock_init);
module_exit(ding_test_afsock_exit);

MODULE_LICENSE("GPL");
