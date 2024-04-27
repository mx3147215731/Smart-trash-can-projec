#ifndef __UARTTOOL_H
#define _UARTTOOL_H

int myserialOpen (const char *device, const int baud);
void serialSendstring (const int fd, const unsigned char *s,int len);
int serialGetstring (const int fd,unsigned char *buffer);

//设置串口5  和 波特率 115200
#define SERIAL_DEV "/dev/ttyS5"
#define BAUD 115200

#endif