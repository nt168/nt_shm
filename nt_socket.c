#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <netinet/in.h> // 用于 sockaddr_in
#include <arpa/inet.h>  // 用于 inet_ntoa, inet_pton
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "nt_cycle.h"
#include "nt_log.h"
#include "nt_ptcl.h"

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024
extern cycle_t *queue;

// 将套接字设置为非阻塞模式
int set_nonblocking(int sockfd) {
    int flags;
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

// 解析命令行参数
int parse_arguments(int argc, char *argv[], char *ip_address, int *port) {
    if (argc != 5) {
        fprintf(stderr, "用法: %s add <IP地址> port <端口号>\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "add") != 0) {
        fprintf(stderr, "第一个参数应为 'add'\n");
        return -1;
    }

    strcpy(ip_address, argv[2]);

    if (strcmp(argv[3], "port") != 0) {
        fprintf(stderr, "第三个参数应为 'port'\n");
        return -1;
    }

    *port = atoi(argv[4]);
    if (*port <= 0 || *port > 65535) {
        fprintf(stderr, "无效的端口号: %d\n", *port);
        return -1;
    }

    return 0;
}

int	is_ip4(const char *ip)
{
	const char	*p = ip;
	int		digits = 0, dots = 0, res = -1, octet = 0;

	while ('\0' != *p)
	{
		if (0 != isdigit(*p))
		{
			octet = octet * 10 + (*p - '0');
			digits++;
		}
		else if ('.' == *p)
		{
			if (0 == digits || 3 < digits || 255 < octet)
				break;

			digits = 0;
			octet = 0;
			dots++;
		}
		else
		{
			digits = 0;
			break;
		}

		p++;
	}
	if (3 == dots && 1 <= digits && 3 >= digits && 255 >= octet)
		res = 0;
	return res;
}

int net_server(const char* add, int port)
{
#if 0
    // 创建工作线程
    pthread_t threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&threads[i], NULL, worker_thread, (void *)queue);
    }
#endif

	int listen_sock, conn_sock, nfds, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct epoll_event ev, events[MAX_EVENTS];
    if(0 != is_ip4(add)){
    	return -1;
    }

    // 创建监听套接字
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // 设置套接字选项，允许地址重用
    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt()");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // 设置监听套接字为非阻塞模式
    if (set_nonblocking(listen_sock) == -1) {
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // 绑定套接字到指定 IP 地址和端口
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // 将 IP 地址从文本形式转换为二进制形式
    if (inet_pton(AF_INET, add, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "无效的 IP 地址: %s\n", add);
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_port = htons(port);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind()");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(listen_sock, SOMAXCONN) == -1) {
        perror("listen()");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // 创建 epoll 实例
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1()");
        close(listen_sock);
        exit(EXIT_FAILURE);
    }

    // 将监听套接字添加到 epoll 实例中
    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        close(listen_sock);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("服务器正在监听 %s:%d...\n", add, port);

    // 事件循环
    while (1) {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // 无限等待
        if (nfds == -1) {
            perror("epoll_wait()");
            break;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_sock) {
                // 处理新连接
                while ((conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len)) > 0) {
                    printf("接受连接：%s:%d\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));

                    // 将新套接字设置为非阻塞模式
                    if (set_nonblocking(conn_sock) == -1) {
                        close(conn_sock);
                        continue;
                    }

                    // 将新套接字添加到 epoll 实例中
                    ev.events = EPOLLIN | EPOLLET; // 边缘触发模式
                    ev.data.fd = conn_sock;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                        perror("epoll_ctl: conn_sock");
                        close(conn_sock);
                        continue;
                    }
                }

                if (conn_sock == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("accept()");
                }

            } else {
                // 处理客户端数据
                int client_fd = events[n].data.fd;
                char buffer[BUFFER_SIZE];

                ssize_t count;
                while (1) {
                    count = read(client_fd, buffer, sizeof(buffer));
                    nt_log(LOG_LEVEL_TRACE, "net_server receive: %ld, %s .", count, buffer);
                    if (count == -1) {
                        if (errno != EAGAIN) {
                            perror("read()");
                            close(client_fd);
                        }
                        break;
                    } else if (count == 0) {
                        // 客户端关闭连接
                        printf("客户端（fd %d）已关闭连接。\n", client_fd);
                        close(client_fd);
                        break;
                    }
#if 0
                    ntptcl_handle(buffer, count);
#endif
#if 1
                	task_t task;
                	memset(&task, 0, sizeof(task));
                	task.data = NULL;
                	task.data = (void*)malloc(count);
                	memset(task.data, 0, count);
                	task.client_fd = client_fd;
                	memcpy(task.data, buffer, count);
                	task.data_len = count;
                	task_queue_push(queue, &task);
                	close(client_fd);
#endif
#if 0
                    // 回显数据给客户端
                    ssize_t write_count = 0;
                    while (write_count < count) {
#if 0
                    	task_t task;
                    	memset(&task, 0, sizeof(task));
                    	task.client_fd = client_fd;
                    	memcpy(task.data, buffer + write_count, count - write_count);
                    	task.data_len = count - write_count;
                    	task_queue_push(queue, &task);
#endif

#if 0
                        ssize_t written = write(client_fd, buffer + write_count, count - write_count);
                        if (written == -1) {
                            perror("write()");
                            close(client_fd);
                            break;
                        }
                        write_count += written;
#endif
                        write_count += (count - write_count);
                    }
#endif
                }
            }
        }
    }

    close(listen_sock);
    close(epoll_fd);
    return 0;
}

unsigned int nt_socket(void* args)
{
	net_server("192.168.43.9", 8912);
	return 0;
}
