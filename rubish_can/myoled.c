
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "oled.h"
#include "font.h"

#define FILENAME "/dev/i2c-3" 


static struct display_info disp;



int myoled_show(void *arg) // oled 显示
{
  unsigned char *buffer = (unsigned char *)arg; // 传入串口数据 -- buffer
  oled_putstrto(&disp, 0, 10, "This garbage is:");// 这垃圾是:
  disp.font = font2; //  指定输出类型
  switch (buffer[2])
  {
  case 0x41:
  oled_putstrto(&disp, 0, 20, "residual(dry) waste");// 干垃圾
  break;

  case 0x42:
  oled_putstrto(&disp, 0, 20, "wet wastee");// 湿垃圾
  break;

  case 0x43:
  oled_putstrto(&disp, 0, 20, "Recyclable waste");// 可回收垃圾
  break;

  case 0x44:
  oled_putstrto(&disp, 0, 20, "hazardous  waste");// 有害垃圾
  break;
  
  case 0x45:
  oled_putstrto(&disp, 0, 20, "Recognition failure");// 识别失败
  break;
  }
  disp.font = font2;
  
  oled_send_buffer(&disp); 

  return 0;
}


int myoled_init(void) //初始化 oled
{
    int e;
    
   
    disp.address = OLED_I2C_ADDR;
    disp.font = font2;

    e = oled_open(&disp, FILENAME);
    e = oled_init(&disp);
    return e;
}
