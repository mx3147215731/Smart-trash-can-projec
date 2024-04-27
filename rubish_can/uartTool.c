#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "wiringSerial.h"
int myserialOpen (const char *device, const int baud)
{
struct termios options ;
speed_t myBaud ;
int status, fd ;
switch (baud){
case 9600: myBaud = B9600 ; break ;
case 115200: myBaud = B115200 ; break ;
}
if ((fd = open (device, O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1)
return -1 ;
fcntl (fd, F_SETFL, O_RDWR) ;
// Get and modify current options:
tcgetattr (fd, &options) ;
cfmakeraw (&options) ;
cfsetispeed (&options, myBaud) ;
cfsetospeed (&options, myBaud) ;
options.c_cflag |= (CLOCAL | CREAD) ;
options.c_cflag &= ~PARENB ;
options.c_cflag &= ~CSTOPB ;
options.c_cflag &= ~CSIZE ;
options.c_cflag |= CS8 ;
options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG) ;
options.c_oflag &= ~OPOST ;
options.c_cc [VMIN] = 0 ;
options.c_cc [VTIME] = 100 ; // Ten seconds (100 deciseconds)
tcsetattr (fd, TCSANOW, &options) ;
ioctl (fd, TIOCMGET, &status);
status |= TIOCM_DTR ;
status |= TIOCM_RTS ;
ioctl (fd, TIOCMSET, &status);
usleep (10000) ; // 10mS

return fd ;
}
//0xAA 0x55  0x64 00  0x55 0xAA  //0x AA > 157 unsigned
void serialSendstring (const int fd, const unsigned char *s,int len)
{
int ret;
ret = write (fd, s, len); //我们有结束位，而且不为'\0',所以不能用strlen
if (ret < 0)
printf("Serial Puts Error\n");
}
int serialGetstring (const int fd,unsigned char *buffer)
{
int n_read;
n_read = read(fd, buffer,32);
return n_read;
}