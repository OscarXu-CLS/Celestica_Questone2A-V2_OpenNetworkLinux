#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include "fancontrol.h"
#include "platform_wobmc.h"

extern int psu1_src, psu2_src;

linear_t U28LINEAR_F2B = {
    41, 54, 90, 230, 3, UNDEF, 0, 0
};

linear_t U29LINEAR_F2B = {
    41, 54, 90, 230, 3, UNDEF, 0, 0
};

linear_t U5LINEAR_B2F = {
    25, 48, 90, 255, 3, UNDEF, 0, 0
};

linear_t U56LINEAR_B2F = {
    25, 48, 90, 255, 3, UNDEF, 0, 0
};

pid_algo_t CPU_PID = {
     UNDEF, 0, 78, 3, 0.5, 0.2, 0
};

pid_algo_t U31_PID_F2B = {
     UNDEF, 0, 57, 3, 0.5, 0.2, 0
};

pid_algo_t U31_PID_B2F = {
     UNDEF, 0, 70, 3, 0.5, 0.5, 0
};


extern const struct psu_reg_bit_mapper psu_mapper [PSU_COUNT + 1];

static short get_temp(const char *path)
{
    short temp = 0;
    int t  = 0;
    int ret = 0, i = 0, cnt = 0;

    /* Read data three times to filter out invalid data */
    for (i = 1; i <= 3; i++) {
        ret = get_sysnode_value(path, (void *)&t);
        if (ret) {
            cnt++;
            if (cnt == 3) {
                printf("Can't get sysnode value\n");
                return TEMP_ERR;
            }
            else
                continue;
        }
        else
            break;
    }

    temp = t / TEMP_CONV_FACTOR;

    return temp;
}

static int set_fan(const char *path, uint8_t pwm)
{
    int ret = 0;

    ret = set_sysnode_value(path, pwm);

    return ret;
}

static int set_all_fans(uint8_t pwm)
{
    int ret = 0, i = 0;
    char  data[100] = {0};

    for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
        sprintf(data, "%spwm%d", CHASSIS_FAN_PATH, i);
        ret = set_fan(data, pwm);
        if (ret)
        {
            printf("Can't set fan%d pwm\n", i);
            return ERR;
        }
    }

    return 0;
}

static int check_psu_present(void)
{
    uint8_t psu_status = 0, present_status = 0;

    psu_status = get_psu_status(PSU1_ID);
    present_status = (psu_status >> psu_mapper[PSU1_ID].bit_present) & 0x01;
    if (1 == present_status) /* absent */
    {
        return FAULT;
    }
    psu_status = get_psu_status(PSU2_ID);
    present_status = (psu_status >> psu_mapper[PSU2_ID].bit_present) & 0x01;
    if (1 == present_status) /* absent */
    {
        return FAULT;
    }

    return OK;
}

static int check_fan_fault(uint8_t *fault_num)
{
    int speed = 0, status = 0;
    int ret = 0, i = 0;
    char data[100] = {0};
    /* to avoid collect error logs all the time */

    *fault_num = 0;

    for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
        sprintf(data, "%sfan%d_present", CHASSIS_FAN_PATH, i);
        ret = get_sysnode_value(data, (void *)&status);
        if (ret)
        {
            printf("Can't get sysnode value\n");
            return ERR;
        }
        if (status == ABSENT)
        {
            return ABSENT;
        }

        sprintf(data, "%sfan%d_front_speed_rpm", CHASSIS_FAN_PATH, i);
        ret = get_sysnode_value(data, (void *)&speed);
        if (ret)
        {
            printf("Can't get sysnode value\n");
            return ERR;
        }
        if (speed < RPM_FAULT_DEF_LC_F)
        {
            (*fault_num)++;
            syslog(LOG_WARNING, "Fan%d front rpm %d is lower than TACH threshold(%d), fault!!!\n",
                    i, speed, RPM_FAULT_DEF_LC_F);
        }

        if (speed > RPM_FAULT_DEF_HC_F)
        {
            (*fault_num)++;
            syslog(LOG_WARNING, "Fan%d front rpm %d is higher than TACH threshold(%d), fault!!!\n",
                    i, speed, RPM_FAULT_DEF_HC_F);
        }

        sprintf(data, "%sfan%d_rear_speed_rpm", CHASSIS_FAN_PATH, i);
        ret = get_sysnode_value(data, (void *)&speed);
        if (ret)
        {
            printf("Can't get sysnode value\n");
            return ERR;
        }

        if (speed < RPM_FAULT_DEF_LC_R)
        {
            (*fault_num)++;
                syslog(LOG_WARNING, "Fan%d rear rpm %d is lower than TACH threshold(%d), fault !!!\n",
                        i, speed, RPM_FAULT_DEF_LC_R);
        }

        if (speed > RPM_FAULT_DEF_HC_F)
        {
            (*fault_num)++;
            syslog(LOG_WARNING, "Fan%d front rpm %d is higher than TACH threshold(%d), fault!!!\n",
                    i, speed, RPM_FAULT_DEF_HC_F);
        }

    }

    if (*fault_num > 1)
        return FAULT;
    else
        return OK;
}

static short upper_bound(short temp, short min_temp, short max_temp, uint8_t min_pwm, uint8_t max_pwm, uint8_t hysteresis)
{
    short pwm = 0;

    pwm = (temp - min_temp) * (max_pwm - min_pwm) / (max_temp - min_temp - hysteresis) + min_pwm;
    //DEBUG_PRINT("%d, %d, %d, %d, %d, %d, %d\n", temp, min_temp, max_temp, min_pwm, max_pwm, hysteresis, pwm);

    return pwm > 0 ? pwm : 0;
}

static short lower_bound(short temp, short min_temp, short max_temp, uint8_t min_pwm, uint8_t max_pwm, uint8_t hysteresis)
{
    short pwm = 0;

    pwm = (temp - min_temp - hysteresis) * (max_pwm - min_pwm) / (max_temp - min_temp - hysteresis) + min_pwm;
    //DEBUG_PRINT("%d, %d, %d, %d, %d, %d, %d\n", temp, min_temp, max_temp, min_pwm, max_pwm, hysteresis, pwm);

    return pwm > 0 ? pwm : 0;
}

static uint8_t linear_cal(short temp, linear_t *l)
{
    uint8_t pwm = 0;
    short ub = 0, lb = 0;

    ub = upper_bound(temp, l->min_temp, l->max_temp, l->min_pwm, l->max_pwm, l->hysteresis);
    lb = lower_bound(temp, l->min_temp, l->max_temp, l->min_pwm, l->max_pwm, l->hysteresis);

    if (UNDEF == l->lasttemp) {
        l->lasttemp = temp;
        l->lastpwm = lb;
        return lb > PWM_START ? lb : PWM_START;
    }
    if (temp >= l->max_temp) {
        l->lasttemp = temp;
        l->lastpwm = PWM_MAX;
        return PWM_MAX;
    } else if (temp < l->min_temp) {
        l->lasttemp = temp;
        l->lastpwm = PWM_MIN;
        return PWM_MIN;
    }

    DEBUG_PRINT("[start] linear temp %d, lasttemp %d\n", temp, l->lasttemp);
    if (temp > l->lasttemp)
    {
        DEBUG_PRINT("temperature rises from %d to %d\n", l->lasttemp, temp);
        if (lb < l->lastpwm)
        {
            pwm = l->lastpwm;
        }
        else
        {
            pwm = lb;
        }
    }
    else if (temp < l->lasttemp)
    {
        DEBUG_PRINT("temperature declines from %d to %d\n", l->lasttemp, temp);
        if (ub > l->lastpwm)
        {
            pwm = l->lastpwm;
        }
        else
        {
            pwm = ub;
        }
    }
    else
    {
        DEBUG_PRINT("temperature keeps to %d\n", temp);
        pwm = l->lastpwm;
    }
    if (pwm > l->max_pwm)
    {
        pwm = l->max_pwm;
    }
    else if (pwm < l->min_pwm)
    {
        pwm = l->min_pwm;
    }
    DEBUG_PRINT("[end] last pwm: %d, new pwm: %d\n", l->lastpwm, pwm);
    l->lasttemp = temp;
    l->lastpwm = pwm;

    return pwm;
}

static uint8_t pid_cal(short temp, pid_algo_t *pid)
{
    int pweight = 0;
    int iweight = 0;
    int dweight = 0;
    /* According to the CPU PID formula of DS2000, it is theoretically possible for PWM to be negative. */
    short pwmpercent = 0;
    uint8_t pwm = 0;

    if (UNDEF == pid->lasttemp) {
        pid->lasttemp = temp;
        pid->lastlasttemp = temp;
        pid->lastpwmpercent = PWM_START * 100 / PWM_MAX;  /* Set 50% pwm as initial pwm on DS2000 */
        return PWM_START;
    }

    pweight = temp - pid->lasttemp;
    iweight = temp - pid->setpoint;
    dweight = temp - 2 * (pid->lasttemp) + pid->lastlasttemp;
    DEBUG_PRINT("[start] pid temp %d, lasttemp %d, lastlasttemp %d\n",
                               temp, pid->lasttemp, pid->lastlasttemp);
    DEBUG_PRINT("P %f, I %f, D %f, pweight %d, iweight %d, dweight %d\n",
                 pid->P, pid->I, pid->D, pweight, iweight, dweight);

    /* Add 0.5 pwm to support rounding */
    pwmpercent = (int)(pid->lastpwmpercent +
                  pid->P * pweight + pid->I * iweight + pid->D * dweight + 0.5);
    DEBUG_PRINT("pid percent before cal %d\n", pwmpercent);
    pwmpercent = pwmpercent < (PWM_MIN * 100 / PWM_MAX) ? (PWM_MIN * 100 / PWM_MAX) : pwmpercent;
    pwmpercent = pwmpercent > 100 ? 100 : pwmpercent;
    DEBUG_PRINT("pid percent after cal %d\n", pwmpercent);
    pwm = pwmpercent * PWM_MAX / 100;
    pid->lastlasttemp = pid->lasttemp;
    pid->lasttemp = temp;
    pid->lastpwmpercent = pwmpercent;
    DEBUG_PRINT("[end] pid pwm %d\n", pwm);

    return pwm;
}

int update_fan(void)
{
    int ret = 0;
    int sw_hc_off_flg = 0, wrn_flg = 0, neg_temp_flg = 0, lowest_temp_flg = 0, unk_src_flg = 0;
    int fan_direction = 0;
    uint8_t pwm = 0, fan_full_speed_fault_num = 0, fault_num = 0;
    static uint8_t fan_status = 0;
    int psu1_hi_warn1 = 0, psu1_hi_warn2 = 0, psu2_hi_warn1 = 0, psu2_hi_warn2 = 0;

    uint8_t fan_out_full_speed_enable = 0;
    uint8_t fan_in_duty_speed_enable = 0;
    uint8_t psu_absent_full_speed_enable = 0;
    uint8_t temper_fail_full_speed_enable = 0;
    uint8_t fan_fault_full_speed_enable = 0;
    uint8_t switch_high_critical_off_enable = 0;
    uint8_t over_temp_full_speed_warn_enable = 0;
    uint8_t lowest_temp_min_speed_warn_enable = 0;
    uint8_t negative_temp_min_speed_enable = 0;
    uint8_t unknown_src_full_speed_enable = 0;
    uint8_t fan_in_duty_speed_percent = 50;
    uint8_t fan_in_duty_speed_duration = 10; // seconds

    short cpu_temp = 0, u31_temp = 0; //PID
    short u28_temp = 0, u29_temp = 0, u5_temp = 0, u56_temp = 0; //LINEAR
    short vdd_core_temp = 0, vdd_anlg_temp = 0;
    short psu1_temp1 = 0, psu1_temp2 = 0;
    short psu2_temp1 = 0, psu2_temp2 = 0;
    short dimma0_temp = 0, dimmb0_temp = 0;
    short neg_tmp_temp = 0, lowest_tmp_temp = 0;

    uint8_t cpu_pwm = 0;
    uint8_t u28_pwm = 0, u29_pwm = 0, u5_pwm = 0, u56_pwm = 0, u31_pwm = 0;
    uint8_t tmp1_pwm = 0, tmp2_pwm = 0;


    /* to avoid collect error logs all the time */
    static uint8_t cputemp_flt_flg = 0;
    static uint8_t u31temp_flt_flg = 0;
    static uint8_t u28temp_flt_flg = 0;
    static uint8_t u29temp_flt_flg = 0;
    static uint8_t u5temp_flt_flg  = 0;
    static uint8_t u56temp_flt_flg = 0;
    static uint8_t vddcoretemp_flt_flg = 0;
    static uint8_t vddanlgtemp_flt_flg = 0;
    static uint8_t psu1temp1_flt_flg = 0;
    static uint8_t psu1temp2_flt_flg = 0;
    static uint8_t psu2temp1_flt_flg = 0;
    static uint8_t psu2temp2_flt_flg = 0;
    static uint8_t dimma0temp_flt_flg = 0;
    static uint8_t dimmb0temp_flt_flg = 0;
    static uint8_t wrn_flt_flg = 0;
    static uint8_t neg_temp_flt_flg = 0;
    static uint8_t lowest_temp_flt_flg = 0;
    static uint8_t unk_src_flt_flg = 0;

    fan_out_full_speed_enable = 1;
    fan_in_duty_speed_enable = 1;
    psu_absent_full_speed_enable = 1;
    temper_fail_full_speed_enable = 1;
    fan_fault_full_speed_enable = 1;
    switch_high_critical_off_enable = 1;
    over_temp_full_speed_warn_enable = 1;
    lowest_temp_min_speed_warn_enable = 1;
    negative_temp_min_speed_enable = 1;
    unknown_src_full_speed_enable = 1;
    fan_full_speed_fault_num = 2;
    fan_in_duty_speed_percent = 70;
    fan_in_duty_speed_duration = 20; // seconds

    /* get all temperatures */
    cpu_temp = get_temp(CPU_Temp);
    u5_temp = get_temp(Base_Temp_U5);
    u56_temp = get_temp(Base_Temp_U56);
    u31_temp = get_temp(Switch_Temp_U31);
    u29_temp = get_temp(Switch_Temp_U29);
    u28_temp = get_temp(Switch_Temp_U28);

    dimma0_temp = get_temp(TEMP_DIMMA0);
    dimmb0_temp = get_temp(TEMP_DIMMB0);
    vdd_core_temp = get_temp(VDD_CORE_Temp);
    vdd_anlg_temp = get_temp(VDD_ANLG_Temp);
    psu1_temp1 = get_temp(PSU1_Temp1);
    psu1_temp2 = get_temp(PSU1_Temp2);
    psu2_temp1 = get_temp(PSU2_Temp1);
    psu2_temp2 = get_temp(PSU2_Temp2);

    /* get fan1 direction as system fan direction */
    ret = get_sysnode_value(CHASSIS_FAN_DIRECTION(1), (void *)&fan_direction);
    if (ret)
    {
        printf("Can't get sysnode value\n");
        return ERR;
    }

    if (temper_fail_full_speed_enable) {
        if (fan_direction == FAN_DIR_F2B) {
            if (TEMP_ERR == cpu_temp || TEMP_ERR == u28_temp || TEMP_ERR == u29_temp || TEMP_ERR == u31_temp) {
            DEBUG_PRINT("fail to get temperature, set full pwm: %d\n\n", PWM_MAX);
            set_all_fans(PWM_MAX);
            return 0;
            }
            else
            DEBUG_PRINT("cpu_temp %hd\nu28_temp %hd\nu29_temp %hd\nu31_temp %hd\n",
                        cpu_temp, u28_temp, u29_temp, u31_temp);
        }
        else if (fan_direction == FAN_DIR_B2F) {
            if (TEMP_ERR == cpu_temp || TEMP_ERR == u5_temp || TEMP_ERR == u56_temp || TEMP_ERR == u31_temp) {
            DEBUG_PRINT("fail to get temperature, set full pwm: %d\n\n", PWM_MAX);
            set_all_fans(PWM_MAX);
            return 0;
            }
            else
            DEBUG_PRINT("cpu_temp %hd\nu5_temp %hd\nu56_temp %hd\nu31_temp %hd\n",
                        cpu_temp, u5_temp, u56_temp, u31_temp);
        }
    }


    /* soft shutdown */
    if (switch_high_critical_off_enable) {
        if (fan_direction == FAN_DIR_F2B) {
            if (u31_temp >= Switch_Temp_U31_F2B_HI_SHUTDOWN / TEMP_CONV_FACTOR) {
                syslog(LOG_WARNING, "Switch board U31 F2B temperature %hd is over than %d, soft shutdown after 30s !!!\n",
                        u31_temp, Switch_Temp_U31_F2B_HI_SHUTDOWN / TEMP_CONV_FACTOR);
                closelog();
                DEBUG_PRINT("Switch board U31 F2B temperature %hd is over than %d, soft shutdown after 30s !!!\n",
                              u31_temp, Switch_Temp_U31_B2F_HI_SHUTDOWN / TEMP_CONV_FACTOR);
                sw_hc_off_flg++;
            }
        }
        else if (fan_direction == FAN_DIR_B2F) {
            if (u31_temp >= Switch_Temp_U31_B2F_HI_SHUTDOWN / TEMP_CONV_FACTOR) {
                syslog(LOG_WARNING, "Switch board U31 B2F temperature %hd is over than %d, soft shutdown after 30s !!!\n",
                        u31_temp, Switch_Temp_U31_B2F_HI_SHUTDOWN / TEMP_CONV_FACTOR);
                closelog();
                DEBUG_PRINT("Switch board U31 B2F temperature %hd is over than %d, soft shutdown after 30s !!!\n",
                              u31_temp, Switch_Temp_U31_B2F_HI_SHUTDOWN / TEMP_CONV_FACTOR);
                sw_hc_off_flg++;
            }
        }

        if (sw_hc_off_flg > 0) {
            /* Wait 30s before reset switch board and reboot OS */
            sleep(30);
            system("sync");

            /* Reset switch board */
            syslog(LOG_DEBUG, "Reset switch board !!!\n");
            DEBUG_PRINT("Now reset switch board...\n\n");

            syslog(LOG_DEBUG, "Power off switch board !!!\n");
            DEBUG_PRINT("Now power off switch board !!!\n\n");
            ret = syscpld_setting(LPC_SWITCH_PWCTRL_REG, SWITCH_OFF);
            if (ret)
            {
                        perror("Fail to power off switch board !!!");
                        return ERR;
            }
            sleep(2);

            syslog(LOG_DEBUG, "Power on switch board !!!\n");
            DEBUG_PRINT("Now power on switch board !!!\n\n");
            ret = syscpld_setting(LPC_SWITCH_PWCTRL_REG, SWITCH_ON);
            if (ret)
            {
                perror("Fail to power on switch board !!!");
                return ERR;
            }
            sleep(2);

            /* Reboot the CPU*/
            syslog(LOG_DEBUG, "Reboot the system !!!\n");
            DEBUG_PRINT("Now reboot the system !!!\n\n");
            sleep(2);
            system("reboot");
            }
    }

    switch (psu1_src)
    {
        case MAIN_SRC:
            psu1_hi_warn1 = PSU_Temp1_HI_WARN / TEMP_CONV_FACTOR;
            psu1_hi_warn2 = PSU_Temp2_HI_WARN / TEMP_CONV_FACTOR;
            break;
        case SECD_SRC:
            psu1_hi_warn1 = PSU_GSP_Temp1_HI_WARN / TEMP_CONV_FACTOR;
            psu1_hi_warn2 = PSU_GSP_Temp2_HI_WARN / TEMP_CONV_FACTOR;
            break;
        default:
            unk_src_flg++;
            break;
    }

    switch (psu2_src)
    {
        case MAIN_SRC:
            psu2_hi_warn1 = PSU_Temp1_HI_WARN / TEMP_CONV_FACTOR;
            psu2_hi_warn2 = PSU_Temp2_HI_WARN / TEMP_CONV_FACTOR;
            break;
        case SECD_SRC:
            psu2_hi_warn1 = PSU_GSP_Temp1_HI_WARN / TEMP_CONV_FACTOR;
            psu2_hi_warn2 = PSU_GSP_Temp2_HI_WARN / TEMP_CONV_FACTOR;
            break;
        default:
            unk_src_flg++;
            break;
    }

    /* Set all chassis fans at full speed if any psu is unknown source */
    if (unknown_src_full_speed_enable) {
        if (unk_src_flg > 0) {
            if (0 == unk_src_flt_flg)
                syslog(LOG_WARNING, "Unknown PSU source !!! Run all chassis fans at full speed !!!\n");
            DEBUG_PRINT("PSU detected from unknown source, run all chassis fans at full speed !!!\n");
            unk_src_flt_flg = 1;
            set_all_fans(PWM_MAX);
            return 0;
        }
        else
            unk_src_flt_flg = 0;
    }

    /* warning and full speed if sensor value is higher than major alarm */
    if (over_temp_full_speed_warn_enable) {
        /* F2B thermal sensor */
        if (fan_direction == FAN_DIR_F2B) {
            if (u28_temp >= Switch_Temp_U28_F2B_HI_WARN / TEMP_CONV_FACTOR) {
                if (0 == u28temp_flt_flg)
                    syslog(LOG_WARNING, "Switch board U28 temperature %hd is over than %d !!!\n",
                            u28_temp, Switch_Temp_U28_F2B_HI_WARN / TEMP_CONV_FACTOR);
                DEBUG_PRINT("Switch board U28 temperature %hd is over than %d !!!\n",
                            u28_temp, Switch_Temp_U28_F2B_HI_WARN / TEMP_CONV_FACTOR);
                wrn_flg++;
                u28temp_flt_flg = 1;
            }
            else
                u28temp_flt_flg = 0;

            if (u29_temp >= Switch_Temp_U29_F2B_HI_WARN / TEMP_CONV_FACTOR) {
                if (0 == u29temp_flt_flg)
                    syslog(LOG_WARNING, "Switch board U29 temperature %hd is over than %d !!!\n",
                            u29_temp, Switch_Temp_U29_F2B_HI_WARN / TEMP_CONV_FACTOR);
                DEBUG_PRINT("Switch board U29 temperature %hd is over than %d !!!\n",
                            u29_temp, Switch_Temp_U29_F2B_HI_WARN / TEMP_CONV_FACTOR);
                wrn_flg++;
                u29temp_flt_flg = 1;
            }
            else
                u29temp_flt_flg = 0;

            if (u31_temp >= Switch_Temp_U31_F2B_HI_WARN / TEMP_CONV_FACTOR) {
                if (0 == u31temp_flt_flg)
                    syslog(LOG_WARNING, "Switch board U31 temperature %hd is over than %d !!!\n",
                            u31_temp, Switch_Temp_U31_F2B_HI_WARN / TEMP_CONV_FACTOR);
                DEBUG_PRINT("Switch board U31 temperature %hd is over than %d !!!\n",
                            u31_temp, Switch_Temp_U31_F2B_HI_WARN / TEMP_CONV_FACTOR);
                wrn_flg++;
                u31temp_flt_flg = 1;
            }
            else
                u31temp_flt_flg = 0;
        }

        /* B2F thermal sensor */
        else if (fan_direction == FAN_DIR_B2F) {
            if (u5_temp >= Base_Temp_U5_B2F_HI_WARN / TEMP_CONV_FACTOR) {
                if (0 == u5temp_flt_flg)
                    syslog(LOG_WARNING, "Base board U5 temperature %hd is over than %d !!!\n",
                            u5_temp, Base_Temp_U5_B2F_HI_WARN / TEMP_CONV_FACTOR);
                DEBUG_PRINT("Base board U5 temperature %hd is over than %d !!!\n",
                            u5_temp, Base_Temp_U5_B2F_HI_WARN / TEMP_CONV_FACTOR);
                wrn_flg++;
                u5temp_flt_flg = 1;
            }
            else
                u5temp_flt_flg = 0;

            if (u56_temp >= Base_Temp_U56_B2F_HI_WARN / TEMP_CONV_FACTOR) {
                if (0 == u56temp_flt_flg)
                    syslog(LOG_WARNING, "Base board U56 temperature %hd is over than %d !!!\n",
                            u56_temp, Base_Temp_U56_B2F_HI_WARN / TEMP_CONV_FACTOR);
                DEBUG_PRINT("Base board U56 temperature %hd is over than %d !!!\n",
                            u56_temp, Base_Temp_U56_B2F_HI_WARN / TEMP_CONV_FACTOR);
                wrn_flg++;
                u56temp_flt_flg = 1;
            }
            else
                u56temp_flt_flg = 0;

            if (u31_temp >= Switch_Temp_U31_B2F_HI_WARN / TEMP_CONV_FACTOR) {
                if (0 == u31temp_flt_flg)
                    syslog(LOG_WARNING, "Switch board U31 temperature %hd is over than %d !!!\n",
                            u31_temp, Switch_Temp_U31_B2F_HI_WARN / TEMP_CONV_FACTOR);
                DEBUG_PRINT("Switch board U31 temperature %hd is over than %d !!!\n",
                            u31_temp, Switch_Temp_U31_B2F_HI_WARN / TEMP_CONV_FACTOR);
                wrn_flg++;
                u31temp_flt_flg = 1;
            }
            else
                u31temp_flt_flg = 0;
        }

        /* Common thermal sensor */
        if (cpu_temp >= CPU_Temp_HI_WARN / TEMP_CONV_FACTOR) {
            if (0 == cputemp_flt_flg)
                syslog(LOG_WARNING, "CPU temperature %hd is over than %d !!!\n",
                        cpu_temp, CPU_Temp_HI_WARN / TEMP_CONV_FACTOR);
            DEBUG_PRINT("CPU temperature %hd is over than %d !!!\n",
                        cpu_temp, CPU_Temp_HI_WARN / TEMP_CONV_FACTOR);
            wrn_flg++;
            cputemp_flt_flg = 1;
        }
        else
            cputemp_flt_flg = 0;

        if (psu1_temp1 >= psu1_hi_warn1) {
            if (0 == psu1temp1_flt_flg)
                syslog(LOG_WARNING, "PSU1 temperature1 %hd is over than %d !!!\n",
                        psu1_temp1, psu1_hi_warn1);
            DEBUG_PRINT("PSU1 temperature1 %hd is over than %d !!!\n",
                        psu1_temp1, psu1_hi_warn1);
            wrn_flg++;
            psu1temp1_flt_flg = 1;
        }
        else
            psu1temp1_flt_flg = 0;

        if (psu1_temp2 >= psu1_hi_warn2) {
            if (0 == psu1temp2_flt_flg)
                syslog(LOG_WARNING, "PSU1 temperature2 %hd is over than %d !!!\n",
                        psu1_temp2, psu1_hi_warn2);
            DEBUG_PRINT("PSU1 temperature2 %hd is over than %d !!!\n",
                        psu1_temp2, psu1_hi_warn2);
            wrn_flg++;
            psu1temp2_flt_flg = 1;
        }
        else
            psu1temp2_flt_flg = 0;

        if (psu2_temp1 >= psu2_hi_warn1) {
            if (0 == psu2temp1_flt_flg)
                syslog(LOG_WARNING, "PSU2 temperature1 %hd is over than %d !!!\n",
                        psu2_temp1, psu2_hi_warn1);
            DEBUG_PRINT("PSU2 temperature1 %hd is over than %d !!!\n",
                        psu2_temp1, psu2_hi_warn1);
            wrn_flg++;
            psu2temp1_flt_flg = 1;
        }
        else
            psu2temp1_flt_flg = 0;

        if (psu2_temp2 >= psu2_hi_warn2) {
            if (0 == psu2temp2_flt_flg)
                syslog(LOG_WARNING, "PSU2 temperature2 %hd is over than %d !!!\n",
                        psu2_temp2, psu2_hi_warn2);
            DEBUG_PRINT("PSU2 temperature2 %hd is over than %d !!!\n",
                        psu2_temp2, psu2_hi_warn2);
            wrn_flg++;
            psu2temp2_flt_flg = 1;
        }
        else
            psu2temp2_flt_flg = 0;

        if (dimma0_temp >= DIMMA0_THERMAL_HI_WARN / TEMP_CONV_FACTOR) {
            if (0 == dimma0temp_flt_flg)
                syslog(LOG_WARNING, "DIMMA0 temperature %hd is over than %d !!!\n",
                        dimma0_temp, DIMMA0_THERMAL_HI_WARN / TEMP_CONV_FACTOR);
            dimma0temp_flt_flg = 1;
        }
        else if(dimma0_temp == TEMP_ERR) {
            syslog(LOG_DEBUG, "No DIMMA0 temperature !!!Please check if the DIMMA0 is insertrd \n");
            dimma0temp_flt_flg = 1;
            }
        else
            dimma0temp_flt_flg = 0;

        if (dimmb0_temp >= DIMMB0_THERMAL_HI_WARN / TEMP_CONV_FACTOR) {
            if (0 == dimmb0temp_flt_flg)
                syslog(LOG_WARNING, "DIMMB0 temperature %hd is over than %d !!!\n",
                        dimmb0_temp, DIMMB0_THERMAL_HI_WARN / TEMP_CONV_FACTOR);
            dimmb0temp_flt_flg = 1;
        }
        else if(dimmb0_temp == TEMP_ERR) {
            syslog(LOG_DEBUG, "No DIMMB0 temperature !!!Please check if the DIMMB0 is insertrd \n");
            dimmb0temp_flt_flg = 1;
            }
        else
            dimmb0temp_flt_flg = 0;

        if (vdd_core_temp >= VDD_CORE_Temp_HI_WARN / TEMP_CONV_FACTOR) {
            if (0 == vddcoretemp_flt_flg)
                syslog(LOG_WARNING, "VDD CORE temperature %hd is over than %d !!!\n",
                        vdd_core_temp, VDD_CORE_Temp_HI_WARN / TEMP_CONV_FACTOR);
            DEBUG_PRINT("VDD CORE temperature %hd is over than %d !!!\n",
                        vdd_core_temp, VDD_CORE_Temp_HI_WARN / TEMP_CONV_FACTOR);
            wrn_flg++;
            vddcoretemp_flt_flg = 1;
        }
        else
            vddcoretemp_flt_flg = 0;

        if (vdd_anlg_temp >= VDD_ANLG_Temp_HI_WARN / TEMP_CONV_FACTOR) {
            if (0 == vddanlgtemp_flt_flg)
                syslog(LOG_WARNING, "VDD ANLG temperature %hd is over than %d !!!\n",
                        vdd_anlg_temp, VDD_ANLG_Temp_HI_WARN / TEMP_CONV_FACTOR);
            DEBUG_PRINT("VDD ANLG temperature %hd is over than %d !!!\n",
                        vdd_anlg_temp, VDD_ANLG_Temp_HI_WARN / TEMP_CONV_FACTOR);
            wrn_flg++;
            vddanlgtemp_flt_flg = 1;
        }
        else
            vddanlgtemp_flt_flg = 0;

        /* set full fan speed if any thermal sensor reaches major alarm threshold */
        if (wrn_flg > 0) {
            if (0 == wrn_flt_flg)
                syslog(LOG_WARNING, "Run the fan at full speed !!!\n");
            DEBUG_PRINT("The temperature of some sensors is too high, run all fans at full speed !!!\n");
            wrn_flt_flg = 1;
            set_all_fans(PWM_MAX);
            return 0;
        }
        else
            wrn_flt_flg = 0;

    }

    /* Set all fan min speed when the air inlet temperature sensor reaches negative temperature */
    if (negative_temp_min_speed_enable) {
        if (fan_direction == FAN_DIR_B2F) {
            neg_tmp_temp = u5_temp < u56_temp ? u5_temp : u56_temp;

            if (u5_temp < u56_temp) {
                if ((neg_tmp_temp < 0) && (neg_tmp_temp > Base_Temp_U5_B2F_LC_WARN / TEMP_CONV_FACTOR)) {
                    if (0 == u5temp_flt_flg)
                        syslog(LOG_WARNING, "Base board U5 temperature %hd is lower than 0 !!!\n",
                                neg_tmp_temp);
                    DEBUG_PRINT("Base board U5 temperature %hd is lower than 0 degree !!!\n",
                                neg_tmp_temp);
                    neg_temp_flg++;
                    u5temp_flt_flg = 1;
                    }
                else
                    u5temp_flt_flg = 0;
                }

            else {
                if ((neg_tmp_temp < 0) && (neg_tmp_temp > Base_Temp_U56_B2F_LC_WARN / TEMP_CONV_FACTOR)) {
                    if (0 == u56temp_flt_flg)
                        syslog(LOG_WARNING, "Base board U56 temperature %hd is lower than 0 !!!\n",
                                neg_tmp_temp);
                    DEBUG_PRINT("Base board U56 temperature %hd is lower than 0 degree !!!\n",
                                neg_tmp_temp);
                    neg_temp_flg++;
                    u56temp_flt_flg = 1;
                    }
                else
                    u56temp_flt_flg = 0;
                }

            if (neg_temp_flg > 0) {
                if (0 == neg_temp_flt_flg)
                    syslog(LOG_WARNING, "U5/U56 temperature is lower than 0 degree, run all fans at min speed (%d PWM) !!!\n", PWM_MIN);
                DEBUG_PRINT("U5/U56 temperature is lower than 0 degree, run all fans at min speed (%d PWM) !!!\n", PWM_MIN);
                neg_temp_flt_flg = 1;
                set_all_fans(PWM_MIN);
                return 0;
            }
            else
                neg_temp_flt_flg = 0;

            }

        else if (fan_direction == FAN_DIR_F2B) {
            neg_tmp_temp = u28_temp < u29_temp ? u28_temp : u29_temp;

            if (u28_temp < u29_temp) {
                if ((neg_tmp_temp < 0) && (neg_tmp_temp > Switch_Temp_U28_F2B_LC_WARN / TEMP_CONV_FACTOR)) {
                    if (0 == u28temp_flt_flg)
                        syslog(LOG_WARNING, "Switch board U28 temperature %hd is lower than 0 !!!\n",
                                neg_tmp_temp);
                    DEBUG_PRINT("Switch board U28 temperature %hd is lower than 0 degree !!!\n",
                                neg_tmp_temp);
                    neg_temp_flg++;
                    u28temp_flt_flg = 1;
                    }
                else
                    u28temp_flt_flg = 0;
                }

            else {
                if ((neg_tmp_temp < 0) && (neg_tmp_temp > Switch_Temp_U29_F2B_LC_WARN / TEMP_CONV_FACTOR)) {
                    if (0 == u29temp_flt_flg)
                        syslog(LOG_WARNING, "Switch board U29 temperature %hd is lower than 0 !!!\n",
                                neg_tmp_temp);
                    DEBUG_PRINT("Switch board U29 temperature %hd is lower than 0 degree !!!\n",
                                neg_tmp_temp);
                    neg_temp_flg++;
                    u29temp_flt_flg = 1;
                    }
                else
                    u29temp_flt_flg = 0;
                }

            if (neg_temp_flg > 0) {
                if (0 == neg_temp_flt_flg)
                    syslog(LOG_WARNING, "U28/U29 temperature is lower than 0 degree, run all fans at min speed (%d PWM) !!!\n", PWM_MIN);
                DEBUG_PRINT("U28/U29 temperature is lower than 0 degree, run all fans at min speed (%d PWM) !!!\n", PWM_MIN);
                neg_temp_flt_flg = 1;
                set_all_fans(PWM_MIN);
                return 0;
            }
            else
                neg_temp_flt_flg = 0;

            }

        }

    /* warning and min speed if sensor value is lower than major alarm */
    if (lowest_temp_min_speed_warn_enable) {
        /* B2F thermal sensor */
        if (fan_direction == FAN_DIR_B2F) {
            lowest_tmp_temp = u5_temp < u56_temp ? u5_temp : u56_temp;

            if (u5_temp < u56_temp) {
                if (lowest_tmp_temp <= Base_Temp_U5_B2F_LC_WARN / TEMP_CONV_FACTOR) {
                    if (0 == u5temp_flt_flg)
                        syslog(LOG_WARNING, "Base board U5 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Base_Temp_U5_B2F_LC_WARN / TEMP_CONV_FACTOR);
                    DEBUG_PRINT("Base board U5 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Base_Temp_U5_B2F_LC_WARN / TEMP_CONV_FACTOR);
                    lowest_temp_flg++;
                    u5temp_flt_flg = 1;
                }
                else
                    u5temp_flt_flg = 0;
                }

            else {
                if (lowest_tmp_temp <= Base_Temp_U56_B2F_LC_WARN / TEMP_CONV_FACTOR) {
                    if (0 == u56temp_flt_flg)
                        syslog(LOG_WARNING, "Base board U56 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Base_Temp_U56_B2F_LC_WARN / TEMP_CONV_FACTOR);
                    DEBUG_PRINT("Base board U56 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Base_Temp_U56_B2F_LC_WARN / TEMP_CONV_FACTOR);
                    lowest_temp_flg++;
                    u56temp_flt_flg = 1;
                }
                else
                    u56temp_flt_flg = 0;
                }

            /* set min fan speed if any thermal sensor reaches major alarm threshold */
            if (lowest_temp_flg > 0) {
                if (0 == lowest_temp_flt_flg)
                    syslog(LOG_WARNING, "Run all fans at low temperature min speed (%d PWM) !!!\n", PWM_LT_MIN);
                DEBUG_PRINT("U5/U56 temperature is lower than %d/%d, run all fans at low temperature min speed (%d PWM) !!!\n",
                Base_Temp_U5_B2F_LC_WARN / TEMP_CONV_FACTOR, Base_Temp_U56_B2F_LC_WARN / TEMP_CONV_FACTOR, PWM_LT_MIN);
                lowest_temp_flt_flg = 1;
                set_all_fans(PWM_LT_MIN);
                return 0;
             }
            else
                lowest_temp_flt_flg = 0;

        }

        /* F2B thermal sensor */
        else if (fan_direction == FAN_DIR_F2B) {
            lowest_tmp_temp = u28_temp < u29_temp ? u28_temp : u29_temp;

            if (u28_temp < u29_temp) {
                if (lowest_tmp_temp <= Switch_Temp_U28_F2B_LC_WARN / TEMP_CONV_FACTOR) {
                    if (0 == u28temp_flt_flg)
                        syslog(LOG_WARNING, "Base board U28 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Switch_Temp_U28_F2B_LC_WARN / TEMP_CONV_FACTOR);
                    DEBUG_PRINT("Base board U28 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Switch_Temp_U28_F2B_LC_WARN / TEMP_CONV_FACTOR);
                    lowest_temp_flg++;
                    u28temp_flt_flg = 1;
                }
                else
                    u28temp_flt_flg = 0;
                }

            else {
                if (lowest_tmp_temp <= Switch_Temp_U29_F2B_LC_WARN / TEMP_CONV_FACTOR) {
                    if (0 == u29temp_flt_flg)
                        syslog(LOG_WARNING, "Base board U29 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Switch_Temp_U29_F2B_LC_WARN / TEMP_CONV_FACTOR);
                    DEBUG_PRINT("Base board U29 temperature %hd is lower than %d !!!\n",
                                lowest_tmp_temp, Switch_Temp_U29_F2B_LC_WARN / TEMP_CONV_FACTOR);
                    lowest_temp_flg++;
                    u29temp_flt_flg = 1;
                }
                else
                    u29temp_flt_flg = 0;
                }

            /* set min fan speed if any thermal sensor reaches major alarm threshold */
            if (lowest_temp_flg > 0) {
                if (0 == lowest_temp_flt_flg)
                    syslog(LOG_WARNING, "Run all fans at low temperature min speed (%d PWM) !!!\n", PWM_LT_MIN);
                DEBUG_PRINT("U28/U29 temperature is lower than %d/%d, run all fans at low temperature min speed (%d PWM) !!!\n",
                Switch_Temp_U28_F2B_LC_WARN / TEMP_CONV_FACTOR, Switch_Temp_U29_F2B_LC_WARN / TEMP_CONV_FACTOR, PWM_LT_MIN);
                lowest_temp_flt_flg = 1;
                set_all_fans(PWM_LT_MIN);
                return 0;
             }
            else
                lowest_temp_flt_flg = 0;

        }

    }

    ret = check_psu_present();
    if (OK != ret && psu_absent_full_speed_enable) {
        DEBUG_PRINT("psu absent, set full pwm: %d\n\n", PWM_MAX);
        set_all_fans(PWM_MAX);
        return 0;
    }
    ret = check_fan_fault(&fault_num);
    if (ABSENT == ret && fan_out_full_speed_enable) {
        DEBUG_PRINT("fan absent, set full pwm: %d\n\n", PWM_MAX);
        set_all_fans(PWM_MAX);
        fan_status = ABSENT;
    }
    else if (ABSENT != ret && ABSENT == fan_status && fan_in_duty_speed_enable) {
        DEBUG_PRINT("fan plug in, set duty pwm: %d\n\n", fan_in_duty_speed_percent * 255 / 100);
        set_all_fans(fan_in_duty_speed_percent * 255 / 100);
        fan_status = PRESENT;
        sleep(fan_in_duty_speed_duration);
    }
    else if (FAULT == ret && fault_num >= fan_full_speed_fault_num && fan_fault_full_speed_enable) {
        DEBUG_PRINT("fan fault, set full pwm: %d\n\n", PWM_MAX);
        set_all_fans(PWM_MAX);
        return 0;
    }
    else {
        /* calculate all PWMs */
        cpu_pwm = pid_cal(cpu_temp, &CPU_PID);
        if (FAN_DIR_F2B == fan_direction) {
            u31_pwm = pid_cal(u31_temp, &U31_PID_F2B);
            tmp1_pwm = u28_pwm = linear_cal(u28_temp, &U28LINEAR_F2B);
            tmp2_pwm = u29_pwm = linear_cal(u29_temp, &U28LINEAR_F2B);
        }
        else if (FAN_DIR_B2F == fan_direction) {
            u31_pwm = pid_cal(u31_temp, &U31_PID_B2F);
            tmp1_pwm = u5_pwm = linear_cal(u5_temp, &U5LINEAR_B2F);
            tmp2_pwm = u56_pwm = linear_cal(u56_temp, &U56LINEAR_B2F);
        }
        /* get max PWM */
        pwm = cpu_pwm > u31_pwm ? cpu_pwm : u31_pwm;
        pwm = pwm > tmp1_pwm ? pwm : tmp1_pwm;
        pwm = pwm > tmp2_pwm ? pwm : tmp2_pwm;
        pwm = pwm > PWM_MAX ? PWM_MAX : pwm;
        /* set max PWM to all fans */
        DEBUG_PRINT("set normal pwm: %d\n\n", pwm);
        set_all_fans(pwm);
    }

    return 0;
}
