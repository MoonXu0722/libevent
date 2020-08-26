#include <stdio.h>
#include <unistd.h>
#include <event.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int sys_err (const char *name)
{
	perror("name");
	return -1;
}
void read_cb (int fd, short events, void *arg)
{
	char msg[4096];
	struct event *ev = (struct event*)arg;
	int len = read(fd, msg, sizeof(msg) -1);
	if (len <= 0) {
		event_free(ev);
		close(fd);
		return;
	}
	msg[len] = 0;
	printf("recv the client %d msg: %s\n", fd, msg);
	for (int i=0; i<strlen(msg); i++)
		msg[i] = toupper(msg[i]);
	//printf("after trans:%s\n", msg);
	////如果第一个连接没处理完后面的连接会等待，直到第一个连接处理完位置。这个对于比较复杂的处理还是要用多线程
	sleep(20);
	write(fd, msg, strlen(msg));	
}

void accept_cb (int fd, short events, void *arg)
{
	evutil_socket_t sockfd;
	struct sockaddr_in client;
	socklen_t len = sizeof(client);
	sockfd = accept (fd, (struct sockaddr*)&client, &len);
	//evutil_make_socket_nonblocking(sockfd);
	printf("accept a client %d\n", sockfd);
	struct event_base *base = (struct event_base*) arg;
	struct event* ev = event_new(NULL, -1, 0, NULL, NULL);
	//if data come
	event_assign(ev, base, sockfd, EV_READ | EV_PERSIST, read_cb, (void *)ev);
	event_add(ev, NULL);
}
int create_tcp_socket (const char *ip, short port, int num)
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	evutil_socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		sys_err("socket");
	evutil_make_listen_socket_reuseable(fd);
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
		sys_err("bind");
	if (listen(fd, num) < 0) 
	{
		close(fd);
		sys_err("listen");
	}
	evutil_make_socket_nonblocking(fd);
	return fd;
}

int main ()
{
	int fd = create_tcp_socket("192.168.233.179", 8888, 10000);
	if (fd == -1)
		sys_err("socket");
	struct event_base *base = event_init();
	////添加客户端连接请求事件
	struct event *ev_listen = event_new(base, fd, EV_READ | EV_PERSIST, accept_cb, base);
	event_add(ev_listen, NULL);
	//程序进入无限循环，等待就绪事件并执行事件处理
	event_base_dispatch(base);
	return 0;
}
