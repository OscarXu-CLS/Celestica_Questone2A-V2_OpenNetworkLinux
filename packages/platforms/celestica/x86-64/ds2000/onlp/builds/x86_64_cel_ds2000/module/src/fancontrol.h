#ifndef _FANCONTROL_H_
#define _FANCONTROL_H_

#include "platform_common.h"

#define INTERVAL              3
#define TEMP_CONV_FACTOR      1000
#define PWM_START             128   // 255 * 50%
#define PWM_MIN               90   // 255 * 35%
#define PWM_MAX               255
#define PWM_LT_MIN            26   // 255 * 10%
#define RPM_FAULT_DEF_LC_R    1370 // less than 1370rpm, regarded as fault
#define RPM_FAULT_DEF_LC_F    1370
#define RPM_FAULT_DEF_HC_R    25375
#define RPM_FAULT_DEF_HC_F    29125
#define UNDEF                 -100


typedef struct linear
{
    uint8_t min_temp;
    uint8_t max_temp;
    uint8_t min_pwm;
    uint8_t max_pwm;
    uint8_t hysteresis;
    char    lasttemp;
    uint8_t lastpwm;
    uint8_t lastpwmpercent;
} linear_t;

typedef struct pid
{
    char lasttemp;
    char lastlasttemp;
    uint32_t setpoint;
    float P;
    float I;
    float D;
    float lastpwmpercent;
} pid_algo_t;

int update_fan(void);

#endif
