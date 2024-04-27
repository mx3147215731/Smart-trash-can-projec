#ifndef __PWM_H
#define __PWM_H

#define PWM_GARBAGE1 5
#define PWM_GARBAGE2 6
#define PWM_GARBAGE3 7
#define PWM_GARBAGE4 8


void pwm_write(int pwm_pin);
void pwm_stop(int pwm_pin); 

#endif