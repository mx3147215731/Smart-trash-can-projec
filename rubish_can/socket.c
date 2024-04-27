#include "socket.h"

int socket_init(const char *ipaddr, const char *ipport) // 网络初始化函数  -- 返回网络文件描述符
{
    int s_fd = -1; // 来接收套接字描述符
    int ret = -1;
    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(struct sockaddr_in));
    // 1. socket -- IPv4  流形式i 0 --默认 TCP
    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == s_fd)
    {
        perror("socket");
        exit(-1);
    }
    // 初始化bind需要的struct
    s_addr.sin_family = AF_INET;
    // htons -- h - host to  ns-net short
    s_addr.sin_port = htons(atoi(ipport));
    // a -- ASCLL
    inet_aton(ipaddr, &s_addr.sin_addr);

    // 3.bind
    ret = bind(s_fd, (struct sockaddr *)&s_addr, sizeof(struct sockaddr_in));
    if (-1 == ret)
    {
        perror("bind");
        exit(-1);
    }
    // listen
    ret = listen(s_fd, 1); // 每次只允许 接入一条客户端 -- 每次运行一个人开垃圾桶 

    if (-1 == ret)
    {
        perror("listen");
        exit(-1);
    }

    return s_fd;
}