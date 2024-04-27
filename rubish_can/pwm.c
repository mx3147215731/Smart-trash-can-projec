#include <wiringPi.h> 
#include <softPwm.h>

// 根据公式:  PWMfreq = 1e6 / (100 * range) ,我们要把pwm周期设置为20ms，对应频率就要等于 50hz，range = 200

void pwm_write(int pwm_pin) //传入引脚
{
   pinMode(pwm_pin,OUTPUT); // pwm 信号配置为输出
   softPwmCreate(pwm_pin,0,200); // range = 200 --> 设置周期为20ms
   softPwmWrite(pwm_pin,10); // 45 度 -- 1ms/20ms -- > 10/200      
   delay(1000); //给 1000 ms  =1 s的延迟，让舵机维持1s
   softPwmStop(pwm_pin);// 关掉高电平 -- 输出低电平，得到想要的pwm波形
}


void pwm_stop(int pwm_pin) //传入引脚
{
   pinMode(pwm_pin,OUTPUT); // pwm 信号配置为输出
   softPwmCreate(pwm_pin,0,200); // range = 200 --> 设置周期为20ms
   softPwmWrite(pwm_pin,5); // 0 度 -- 0.5ms/20ms -- > 5/200      
   delay(1000); //给 1000 ms  =1 s的延迟，让舵机维持1s
   softPwmStop(pwm_pin);// 关掉高电平 -- 输出低电平，得到想要的pwm波形
}

