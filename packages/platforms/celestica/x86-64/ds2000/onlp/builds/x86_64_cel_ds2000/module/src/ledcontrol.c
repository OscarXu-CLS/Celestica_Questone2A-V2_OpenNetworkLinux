#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <syslog.h>

#include "platform_common.h"
#include "platform_wobmc.h"
#include "ledcontrol.h"

extern const struct psu_reg_bit_mapper psu_mapper [PSU_COUNT + 1];

static uint8_t set_psu_led(uint8_t mode)
{
    int ret = 0;

    ret = syscpld_setting(LPC_LED_PSU_REG, mode);
    if (ret)
    {
        perror("Can't set PSU LED");
        return ERR;
    }

    return OK;
}

static uint8_t set_fan_led(uint8_t mode)
{
    int ret = 0;

    ret = syscpld_setting(LPC_LED_FAN_REG, mode);
    if (ret)
    {
        perror("Can't set FAN LED");
        return ERR;
    }

    return OK;
}

static int set_chassis_fan_led(int fan_num, uint8_t mode)
{
    int ret = 0;

    switch (fan_num)
    {
        case CHASSIS_FAN1_ID:
            ret = syscpld_setting(LPC_FAN1_LED_REG, mode);
            if (ret)
            {
                syslog(LOG_WARNING, "Can't set FAN%d LED", fan_num);
                return ERR;
            }
            break;
        case CHASSIS_FAN2_ID:
            ret = syscpld_setting(LPC_FAN2_LED_REG, mode);
            if (ret)
            {
                syslog(LOG_WARNING, "Can't set FAN%d LED", fan_num);
                return ERR;
            }
            break;
        case CHASSIS_FAN3_ID:
            ret = syscpld_setting(LPC_FAN3_LED_REG, mode);
            if (ret)
            {
                syslog(LOG_WARNING, "Can't set FAN%d LED", fan_num);
                return ERR;
            }
            break;
        case CHASSIS_FAN4_ID:
            ret = syscpld_setting(LPC_FAN4_LED_REG, mode);
            if (ret)
            {
                syslog(LOG_WARNING, "Can't set FAN%d LED", fan_num);
                return ERR;
            }
            break;
        default:
            printf("Invalid Fan ID %d\n", fan_num);
            break;
    }

    return OK;
}

static int check_psu_status(void)
{
    uint8_t i = 0;
    uint8_t psu_status = 0;
    /* to avoid collect error logs all the time */
    static uint8_t dc_flt_flg = 0;
    static uint8_t absent_flg = 0;

    int present_status=0, pwrok_status=0;

    for (i = 1; i <= PSU_COUNT; i++)
    {
        psu_status = get_psu_status(i);
        present_status = (psu_status >> psu_mapper[i].bit_present) & 0x01;
        pwrok_status = (psu_status >> psu_mapper[i].bit_pwrok_sta) & 0x01;

        if (0 == present_status)
        {
            if (0 == pwrok_status)
            {
                if (0 == (dc_flt_flg & (1 << i)))
                    syslog(LOG_WARNING, "PSU%d DC power is detected failed !!!\n", i);
                dc_flt_flg |= (1 << i);
                return ERR;
            }
            else
            {
                dc_flt_flg &= ~(1 << i);
            }
            absent_flg &= ~(1 << i);
        }
        else
        {
            if (0 == (absent_flg & (1 << i)))
                syslog(LOG_WARNING, "PSU%d is absent !!!\n", i);
            absent_flg |= (1 << i);
            return ERR;
        }
    }

    return OK;
}


static int check_fan_status(void)
{
    int speed = 0, status = 0;
    int ret = 0, i = 0, cnt = 0;
    int pwm = 0;
    char data[100] = {0};
    int low_thr, high_thr = 0;
    int sys_fan_direction = 0, fan_direction = 0;

    /* to avoid collect error logs all the time */
    static uint8_t frtovspeed_flg = 0;
    static uint8_t frtudspeed_flg = 0;
    static uint8_t rearovspeed_flg = 0;
    static uint8_t rearudspeed_flg = 0;
    static uint8_t absent_flg = 0;

    /* get fan1 direction as system fan direction */
    ret = get_sysnode_value(CHASSIS_FAN_DIRECTION(1), (void *)&sys_fan_direction);
    if (ret)
    {
        printf("Can't get sysnode value\n");
        return ERR;
    }

    for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
        switch (i) 
        {
            case CHASSIS_FAN1_ID:
                ret = get_sysnode_value(CHASSIS_FAN_DIRECTION(1), (void *)&fan_direction);
                if (ret)
                {
                    printf("Can't get Fan%d direction!\n", i);
                    return ERR;
                }
                break;
            case CHASSIS_FAN2_ID:
                ret = get_sysnode_value(CHASSIS_FAN_DIRECTION(2), (void *)&fan_direction);
                if (ret)
                {
                    printf("Can't get Fan%d direction!\n", i);
                    return ERR;
                }
                break;
            case CHASSIS_FAN3_ID:
                ret = get_sysnode_value(CHASSIS_FAN_DIRECTION(3), (void *)&fan_direction);
                if (ret)
                {
                    printf("Can't get Fan%d direction!\n", i);
                    return ERR;
                }
                break;
            case CHASSIS_FAN4_ID:
                ret = get_sysnode_value(CHASSIS_FAN_DIRECTION(4), (void *)&fan_direction);
                if (ret)
                {
                    printf("Can't get Fan%d direction!\n", i);
                    return ERR;
                }
                break;
            default:
                printf("Invalid Fan ID %d\n", i);
                break;
        }

        sprintf(data, "%sfan%d_present", CHASSIS_FAN_PATH, i);
        ret = get_sysnode_value(data, (void *)&status);
        if (ret)
        {
            printf("Can't get Fan%d present status!\n", i);;
            return ERR;
        }

        if (status == ABSENT)
        {
            if (0 == (absent_flg & (1 << i)))
                syslog(LOG_WARNING, "Fan%d is absent !!!\n", i);

            absent_flg |= (1 << i);

            /* Fan absent, set chassis Fan LED 'OFF' */
            set_chassis_fan_led(i, FAN_LED_OFF);

            /* if all fans are absent, set fan LED as off */
            cnt++;
            if (cnt == CHASSIS_FAN_COUNT)
                return ABSENT;
            continue;
        }
        else
            absent_flg &= ~(1 << i);

        if (cnt > 0)
            return ERR;


        if (status == PRESENT) {
            /*The airflow status is not equal sys airflow, set chassis Fan LED 'Amber' */
            if (fan_direction != sys_fan_direction) {
                syslog(LOG_WARNING,
                    "Fan%d direction(%d) is inconsistent with sys Fan direction, set chassis Fan%d LED to 'Amber/Yellow'\n",
                    i, fan_direction, i);
                set_chassis_fan_led(i, FAN_LED_AMB);
                return ERR;
                }

            sprintf(data, "%sfan%d_front_speed_rpm", CHASSIS_FAN_PATH, i);
            ret = get_sysnode_value(data, (void *)&speed);
            if (ret)
            {
                printf("Can't get Fan%d front RPM!\n", i);
                return ERR;
            }
            if (speed < RPM_FAULT_DEF_LC_F) {
                syslog(LOG_WARNING,
                    "Fan%d front speed is lower than %dRPM, set chassis Fan%d LED to 'Amber/Yellow'\n",
                    i, RPM_FAULT_DEF_LC_F, i);
                set_chassis_fan_led(i, FAN_LED_AMB);
                return ERR;
            }

            /* if fan rpm duty is out of tolerant scope, set as fault */
            sprintf(data, "%spwm%d", CHASSIS_FAN_PATH, i);
            ret = get_sysnode_value(data, (void *)&pwm);
            if (ret)
            {
                printf("Can't get Fan%d PWM!\n", i);
                return ERR;
            }
            low_thr = CHASSIS_FAN_FRONT_MAX_RPM * pwm * (100 - CHASIS_FAN_TOLERANT_PER) / 25500;
            high_thr = CHASSIS_FAN_FRONT_MAX_RPM * pwm * (100 + CHASIS_FAN_TOLERANT_PER) / 25500;
            if (speed < low_thr)
            {
                if (0 == (frtudspeed_flg & (1 << i)))
                    syslog(LOG_WARNING, "Fan%d front rpm %d is less than %d at pwm %d!!!\n",
                                    i, speed, low_thr, pwm);
                frtudspeed_flg |= (1 << i);
                return ERR;
            }
            else if (speed > high_thr)
            {
                if (0 == (frtovspeed_flg & (1 << i)))
                    syslog(LOG_WARNING, "Fan%d front rpm %d is larger than %d at pwm %d!!!\n",
                                    i, speed, high_thr, pwm);
                frtovspeed_flg |= (1 << i);
                return ERR;
            }
            else
            {
                frtovspeed_flg &= ~(1 << i);
                frtudspeed_flg &= ~(1 << i);
            }

            sprintf(data, "%sfan%d_rear_speed_rpm", CHASSIS_FAN_PATH, i);
            ret = get_sysnode_value(data, (void *)&speed);
            if (ret)
            {
                printf("Can't get Fan%d rear RPM!\n", i);
                return ERR;
            }
            if (speed < RPM_FAULT_DEF_LC_R) {
                syslog(LOG_WARNING,
                    "Fan%d rear speed is lower than %dRPM, set chassis Fan%d LED to 'Amber/Yellow'\n",
                    i, RPM_FAULT_DEF_LC_R, i);
                set_chassis_fan_led(i, FAN_LED_AMB);
                return ERR;
            }

            /* if fan rpm duty is out of tolerant scope, set as fault */
            low_thr = CHASSIS_FAN_REAR_MAX_RPM * pwm * (100 - CHASIS_FAN_TOLERANT_PER) / 25500;
            high_thr = CHASSIS_FAN_REAR_MAX_RPM * pwm * (100 + CHASIS_FAN_TOLERANT_PER) / 25500;
            if (speed < low_thr)
            {
                if (0 == (rearudspeed_flg & (1 << i)))
                    syslog(LOG_WARNING, "Fan%d rear rpm %d is less than %d at pwm %d!!!\n",
                                    i, speed, low_thr, pwm);
                rearudspeed_flg |= (1 << i);
                return ERR;
            }
            else if (speed > high_thr)
            {
                if (0 == (rearovspeed_flg & (1 << i)))
                    syslog(LOG_WARNING, "Fan%d rear rpm %d is larger than %d at pwm %d!!!\n",
                                    i, speed, high_thr, pwm);
                rearovspeed_flg |= (1 << i);
                return ERR;
            }
            else
            {
                rearovspeed_flg &= ~(1 << i);
                rearudspeed_flg &= ~(1 << i);
            }

            set_chassis_fan_led(i, FAN_LED_GRN);

            }
    }

    return OK;
}

int update_led(void)
{
    uint8_t ret = 0;

    ret = check_psu_status();
    if (OK == ret)
    {
        /* set psu led as green */
        set_psu_led(LED_PSU_GRN);
    }
    else
    {
        /* set psu led as amber */
        set_psu_led(LED_PSU_AMB);
    }

    ret = check_fan_status();
    if (OK == ret)
    {
        /* set fan led as green */
        set_fan_led(LED_FAN_GRN);
    }
    else if (ABSENT == ret)
    {
        /* set fan led as off */
        set_fan_led(LED_FAN_OFF);
    }
    else
    {
        /* set fan led as amber */
        set_fan_led(LED_FAN_AMB);
    }

    return 0;
}
