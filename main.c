#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // access()
#include <error.h>  // remove()
#include <wiringPi.h>
#include <softPwm.h>
#include <pthread.h>
#include<netinet/tcp.h> // 心跳包  -- 需要的头文件

#include "uartTool.h"
#include "garbage.h"
#include "pwm.h"
#include "myoled.h"
#include "socket.h"

static int detect_process(const char *process_name) // 判断进程是否在运行
{
    int n = -1;
    FILE *strm;
    char buf[128] = {0};
    sprintf(buf, "ps -ax | grep %s|grep -v grep", process_name); // 组合进程名字，为完整命令
    if ((strm = popen(buf, "r")) != NULL)                        // 通过 popen 的 方式去执行
    {
        if (fgets(buf, sizeof(buf), strm) != NULL) // 执行完后 判断是否能拿到正确的进程号，空格分开，第一个字符串就是进程号
        {
            n = atoi(buf); // 拿到就放回 进程号，不然  返回 -1
        }
    }
    else
    {
        return -1; // 执行失败
    }
    pclose(strm);
    return n;
}

int serial_fd = -1;    // 线程调用 -- 定义为全局
pthread_cond_t cond;   // 设置条件变量
pthread_mutex_t mutex; //  设置线程锁

void *pget_voice(void *arg) // 语言播放线程函数
{
    int len = 0;
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0x55, 0xAA}; // 初始化 buffer[2] --  关联垃圾类型
    if (-1 == serial_fd)
    {
        printf("%s | %s | %d:open serial failed\n", __FILE__, __func__, __LINE__); // 三个宏的含义: 文件名 - main.c,函数名 - pget_voice ,行号 -  138
        pthread_exit(0);                                                           // 串口打开失败 -->退出
    }

    while (1)
    {
        len = serialGetstring(serial_fd, buffer); // 通过串口获得语言输入
        if (len > 0 && buffer[2] == 0x46)         // 判断是否 接到识别指令
        {
            pthread_mutex_lock(&mutex);   //  先上锁，保证后面执行这块不会被打断
            buffer[2] = 0x00;             // 判断完后的复位
            pthread_cond_signal(&cond);   // 发送信号，告诉阿里云线程开始识别了
            pthread_mutex_unlock(&mutex); // 解锁，与上锁包含的代码块执行的时候不会被打断
        }
    }

    pthread_exit(0);
}

void *popen_trash_can(void *arg) // 开盖
{
    pthread_detach(pthread_self());
    unsigned char *buffer = (unsigned char *)arg;

    if (buffer[2] == 0x41) // 干垃圾 -- 对应的舵机 -- 垃圾桶
    {
        pwm_write(PWM_GARBAGE1);
        delay(2000);            // 开盖5s
        pwm_stop(PWM_GARBAGE1); // 停止写入波形
    }
    else if (buffer[2] == 0x42)// 湿垃圾 -- 对应的舵机 -- 垃圾桶
    {
        pwm_write(PWM_GARBAGE2);
        delay(2000);            // 开盖5s
        pwm_stop(PWM_GARBAGE2); // 停止写入波形
    }
        else if (buffer[2] == 0x43)// 可回收垃圾 -- 对应的舵机 -- 垃圾桶
    {
        pwm_write(PWM_GARBAGE3);
        delay(2000);            // 开盖5s
        pwm_stop(PWM_GARBAGE3); // 停止写入波形
    }
        else if (buffer[2] == 0x44)// 有害垃圾 -- 对应的舵机 -- 垃圾桶
    {
        pwm_write(PWM_GARBAGE4);
        delay(2000);            // 开盖5s
        pwm_stop(PWM_GARBAGE4); // 停止写入波形
    }
     else if (buffer[2] == 0x45)// 未识别垃圾类型 - 识别不到有害垃圾，打开这个代替 -- 对应的舵机 -- 垃圾桶
    {
        pwm_write(PWM_GARBAGE4);
        delay(2000);            // 开盖5s
        pwm_stop(PWM_GARBAGE4); // 停止写入波形
    }
    



    pthread_exit(0);
}

void *psend_voice(void *arg) // 发送语言播报
{
    pthread_detach(pthread_self()); // pthread_self -- 拿到自己的线程id --> 与父进程分离，不然开盖等待时间太长影响下一次识别

    unsigned char *buffer = (unsigned char *)arg;
    if (-1 == serial_fd) // 判断串口是否打开
    {
        printf("%s | %s | %d:open serial failed\n", __FILE__, __func__, __LINE__); // 三个宏的含义: 文件名 - main.c,函数名 - pget_voice ,行号 -  138
        pthread_exit(0);                                                           // 串口打开失败 -->退出
    }

    printf("buffer[2] = 0x%x\n", buffer[2]);
    if (NULL != buffer)                         // 有数据
        serialSendstring(serial_fd, buffer, 6); // 将识别到的数据发送到串口,回传给语音模块，语言模块收到数据后进行相应输出 -- 实现语言播报

    pthread_exit(0);
}

void *poled_show(void *arg)
{
    pthread_detach(pthread_self());
    myoled_init();
    myoled_show(arg); // 将buffer 传进来，用于oled的显示

    pthread_exit(0);
}

void *pcategory(void *arg) // 阿里云 -- 垃圾类型识别线程函数
{

    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0x55, 0xAA}; // 初始化 buffer[2] --  关联垃圾类型
    char *category = NULL;
    pthread_t send_voice_tid, trash_tid, oled_tid;

    while (1)
    {
        pthread_mutex_lock(&mutex);       // 拿锁
        pthread_cond_wait(&cond, &mutex); // 等待 接受到信号 -- 才能开始识别
        pthread_mutex_unlock(&mutex);
        // 开始识别
        buffer[2] = 0x00; // 拍照前复位
        system(WGET_CMD); // 拍照

        if (access(GARBAGE_FILE, F_OK) == 0) // 判断 文件存在
        {
            category = garbage_category(category); // 通过通过阿里云接口图像识别 获取垃圾类型

            if (strstr(category, "干垃圾"))
            {
                buffer[2] = 0x41;
            }
            else if (strstr(category, "湿垃圾"))
            {
                buffer[2] = 0x42;
            }
            else if (strstr(category, "可回收垃圾"))
            {
                buffer[2] = 0x43;
            }
            else if (strstr(category, "有害垃圾"))
            {
                buffer[2] = 0x44;
            }
            else
            {
                buffer[2] = 0x45;
            }
        }
        else
        { // 没有获取到图片
            buffer[2] = 0x45;
            //}
        }

        // 创建打开垃圾桶线程
        pthread_create(&trash_tid, NULL, popen_trash_can, (void *)buffer);

        // 创建语音播报线程
        pthread_create(&send_voice_tid, NULL, psend_voice, (void *)buffer);

        // 创建oled显示线程
        pthread_create(&oled_tid, NULL, poled_show, (void *)buffer);

        // buffer[2] = 0x00;                       // 发送完后，一堆有效数据位清零，方便下一次调用
        remove(GARBAGE_FILE); // 清理缓存 删除刚刚拍摄的图片，避免对下一次拍摄造成干扰
    }
    pthread_exit(0);
}

void *pget_socket(void *arg) // 网络控制线程
{
    int s_fd = -1;
    int c_fd = -1;
    char buffer[6];
    int n_read = -1;

    struct sockaddr_in c_addr;
    memset(&c_addr, 0, sizeof(struct sockaddr_in));

    s_fd = socket_init(IPADDR, IPPORT);
    int clen = sizeof(struct sockaddr_in);

    while (1) // 一直等待接收
    {
        c_fd = accept(s_fd, (struct sockaddr *)&c_addr, &clen); // 获得新的客户端 描述符
        int keepalive = 1;                                      // 开启TCP KeepAlive功能
        int keepidle = 5;                                       // tcp_keepalive_time 3s内没收到数据开始发送心跳包
        int keepcnt = 3;                                        // tcp_keepalive_probes 每次发送心跳包的时间间隔,单位秒
        int keepintvl = 3;                                      // tcp_keepalive_intvl 每3s发送一次心跳包
        setsockopt(c_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive,
                   sizeof(keepalive));
        setsockopt(c_fd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepidle, sizeof(keepidle));
        setsockopt(c_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcnt, sizeof(keepcnt));
        setsockopt(c_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof(keepintvl));

        // 打印调试信息
        printf("%s | %s | %d: Access a connection from %s:%d\n", __FILE__, __func__, __LINE__, inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));

        if (c_fd == -1)
        {
            perror("accept");
            continue;
        }

        while (1)
        {
            memset(buffer, 0, sizeof(buffer));
            n_read = recv(c_fd, buffer, sizeof(buffer), 0); // 等待接收
            printf("%s | %s | %d: n_read=%d  buffer=%s\n", __FILE__, __func__, __LINE__, n_read, buffer);

            if (n_read > 0)
            {
                if (strstr(buffer, "open"))
                {
                    // 读取成功 -- 发送 信号
                    pthread_mutex_lock(&mutex); //  先上锁，保证后面执行这块不会被打断
                    buffer[2] = 0x00;           // 判断完后的复位
                    pthread_cond_signal(&cond); // 发送信号，告诉阿里云线程开始识别了
                    pthread_mutex_unlock(&mutex);
                }
            }
            else if (0 == n_read || -1 == n_read) // 没读到，or 读到空
            {
                break;
            }
        }

        close(c_fd);
    }

    pthread_exit(0);
}

int main(int argc, char **argv)
{

    int len = 0;
    int ret = -1;
    char *category = NULL;
    pthread_t get_voice_tid, category_tid, socket_tid; // 创建线程id

    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0x55, 0xAA}; // 初始化 buffer[2] --  关联垃圾类型
    wiringPiSetup();                                                // 初始化wiringPi库
    garbage_init();                                                 // 初始化 阿里云接口
    ret = detect_process("mjpg_streamer");                          // 用于判断mjpg_streamer服务是否已经启动
    if (-1 == ret)
    {
        puts("detect process failed");
        goto END;
    }

    serial_fd = myserialOpen(SERIAL_DEV, BAUD); // 初始化串口,打开串口设备(语言模块)

    if (serial_fd == -1)
    { // 初始化串口失败
        goto END;
    }
    // 创建语音线程  -- 注意第一个参数类型是指针变量 pthread_t *
    pthread_create(&get_voice_tid, NULL, pcategory, NULL);

    // 创建网络控制线程线程
    pthread_create(&socket_tid, NULL, pget_socket, NULL);

    // 创建阿里云交互线程
    pthread_create(&category_tid, NULL, pget_voice, NULL);

    // 第二个参数表示接收到的返回值 -- 没有就NULL
    pthread_join(get_voice_tid, NULL); // 等待线程退出
    pthread_join(category_tid, NULL);
    pthread_join(socket_tid, NULL);

    pthread_mutex_destroy(&mutex); // 释放锁
    pthread_cond_destroy(&cond);   // 释放条件变量
    close(serial_fd);              // 关闭串口文件描述符  fd
END:
    garbage_final();

    return 0;
}