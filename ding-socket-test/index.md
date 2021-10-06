##

>> 启动 socket 服务器
>> 然后 strace -ff -o out node socket.test.js 启动服务
>>> -f 跟踪由fork调用所产生的子进程. 
>>> -ff 如果提供-o filename,则所有进程的跟踪结果输出到相应的filename.pid中,pid是各进程的进程号.
>>>> 所以当前目录会产生多个 out.xxxx 就表示 node 启动的服务是个多线程的, 底层进行了多次 fork
>> netstat -ntlp | grep 8090
>>> tcp6 0 0 :::8090 :::* LISTEN 887746/node 启动 887746 是 PID 号
>> lsof -p 887746 losf 加上 -p 跟 pid 号能把该进程所有打开的文件列出来
>>> lsof -p 887746 | grep 8090
>>>> node 887746 root 20u IPv6 223697955 0t0 TCP *:8090 (LISTEN) 其中 20u 表示文件描述符 0t0 表示偏移量 IPv6 表示 type
>> 然后 tail -f 10 ./out.887746 可以直接查看后十行
>>> 肯定会有调用 socket 系统调用, 然后返回一个文件描述符, 之后 bind 以及 listen
