#ifndef __SOCKET_H_
#define __SOCKET_H_

#include <stdio.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
// #include<linux/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#define IPADDR "192.168.1.11"
#define IPPORT "8888" // 注意不能用 8080  -- 摄像头占用了

int socket_init(const char*ipaddr,const char*ipport);



#endif