//////////////////////////////////////////////////////////////
//   PLATFORM FUNCTION TO INTERACT WITH SYS_CPLD AND BMC    //
//////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>

/*
 * linux/i2c-dev.h provided by i2c-tools contains the symbols defined in linux/i2c.h.
 * This is not usual, but some distros like OpenSuSe does it. The i2c.h will be only
 * included if a well-known symbol is not defined, it avoids to redefine symbols and
 * breaks the build.(refer issue#96)
 */
#ifndef I2C_FUNC_I2C
#include <linux/i2c.h>
#endif

#include <onlplib/i2c.h>
#include "platform_common.h"
#include "platform_wobmc.h"

char command[256];
extern int psu1_src, psu2_src;

int read_smbus_block(int bus, int address, int bank, unsigned char *cblock)
{
    int res = 0, file;
    char filename[20];

    snprintf(filename, sizeof(filename), "/dev/i2c-%d", bus);
    file = open(filename, O_RDWR);
    if (file < 0) {
          return -1;
    }
    if (ioctl(file, I2C_SLAVE_FORCE, address) < 0) {
          return -1;
    }
    res = i2c_smbus_read_block_data(file, bank, cblock);
    close(file);

    return res;
}

int read_eeprom(char* path, int offset, char* data, unsigned int *count)
{

    int ret = 0;

    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        printf("Fail to open the file: %s \n", path);
        return -1;
    }

    if(offset > 0) {
        lseek(fd, offset, SEEK_SET);
    }

    size_t len = read (fd, data, *count);

    if (len == 0) {
        printf("The file(%s) is empty, count=%d. \n", path, *count);
    } else if (len < 0) {
        perror("Can't read eeprom");
        return ret;
    }

    *count = len;
    close(fd);

    return ret;
}

int get_sysnode_value(const char *path, void *data)
{
    int fd, ret = 0;
    FILE *fp = NULL;
    char new_path[200] = {0};
    char buffer[50] = {0};
    int *value = (int *)data;

    /* replace hwmon* with hwmon[0-9] */
    sprintf(new_path, "ls %s", path);
    fp = popen(new_path, "r");
    if (fp == NULL)
    {
        printf("call popen fail\n");
        return -1;
    }
    fscanf(fp, "%s", new_path);
    pclose(fp);

    fd = open(new_path, O_RDONLY);
    if (fd < 0) {
        ret = -1;
    } else {
        read(fd, buffer, sizeof(buffer));
        *value = atoi(buffer);
        close(fd);
    }

    return ret;
}


int set_sysnode_value(const char *path, int data)
{
    int fd, ret = 0;
    FILE *fp = NULL;
    char new_path[200] = {0};
    char buffer[50] = {0};

    /* replace hwmon* with hwmon[0-9] */
    sprintf(new_path, "ls %s", path);
    fp = popen(new_path, "r");
    if (fp == NULL)
    {
        printf("call popen fail\n");
        return -1;
    }
    fscanf(fp, "%s", new_path);
    pclose(fp);

    fd = open(new_path, O_WRONLY);
    if(fd < 0){
        ret = -1;
    }else{
        sprintf(buffer, "%d", data);
        write(fd, buffer, strlen(buffer));
        close(fd);
    }

    return ret;
}

int get_psu_info_wobmc(int id, int *mvin, int *mvout, int *mpin, int *mpout, int *miin, int *miout)
{
    int ret = 0, fail = 0;

    *mvin = *mvout = *mpin = *mpout = *miin = *miout = 0;

    if ((NULL == mvin) || (NULL == mvout) ||(NULL == mpin) || (NULL == mpout) || (NULL == miin) || (NULL == miout)) {
        printf("%s null pointer!\n", __FUNCTION__);
        return -1;
    }
    if (id == 1) {
        /* voltage in and out */
        ret = get_sysnode_value(PSU1_VIN, mvin);
        if (ret < 0)
            fail++;
        ret = get_sysnode_value(PSU1_VOUT, mvout);
        if (ret < 0)
            fail++;
        /* power in and out */
        ret = get_sysnode_value(PSU1_PIN, mpin);
        if (ret < 0)
            fail++;
        *mpin = *mpin / 1000;
        ret = get_sysnode_value(PSU1_POUT, mpout);
        if (ret < 0)
            fail++;
        *mpout = *mpout / 1000;
        /* current in and out*/
        ret = get_sysnode_value(PSU1_CIN, miin);
        if (ret < 0)
            fail++;
        ret = get_sysnode_value(PSU1_COUT, miout);
        if (ret < 0)
            fail++;
    } else {
        /* voltage in and out */
        ret = get_sysnode_value(PSU2_VIN, mvin);
        if (ret < 0)
            fail++;
        ret = get_sysnode_value(PSU2_VOUT, mvout);
        if (ret < 0)
            fail++;
        /* power in and out */
        ret = get_sysnode_value(PSU2_PIN, mpin);
        if (ret < 0)
            fail++;
        *mpin = *mpin / 1000;
        ret = get_sysnode_value(PSU2_POUT, mpout);
        if (ret < 0)
            fail++;
        *mpout = *mpout / 1000;
        /* current in and out*/
        ret = get_sysnode_value(PSU2_CIN, miin);
        if (ret < 0)
            fail++;
        ret = get_sysnode_value(PSU2_COUT, miout);
        if (ret < 0)
            fail++;
    }

    return fail;
}

int get_psu_model_sn_wobmc(int id, char *model, char *serial_number)
{
    unsigned char cblock[288] = {0};
    int ret = 0;
    int bus = 0, address = 0;
    int bank = 0;

    if (id == 1) {
        bus = PSU1_I2C_BUS;
        address = PSU1_I2C_ADDR;
    } else {
        bus = PSU2_I2C_BUS;
        address = PSU2_I2C_ADDR;
    }
    bank = PMBUS_MODEL_REG;
    ret = read_smbus_block(bus, address, bank, cblock);
    if (ret > 0)
        strcpy(model, (char *)cblock);
    else
        return -1;

    memset(cblock,0,sizeof(cblock));
    bank = PMBUS_SERIAL_REG;
    ret = read_smbus_block(bus, address, bank, cblock);
    if (ret > 0)
        strcpy(serial_number, (char *)cblock);
    else
        return -1;

    return 0;
}

int get_fan_info_wobmc(int id, char *model, char *serial, int *isfanb2f)
{
    uint16_t offset = FRU_COMM_HDR_LEN + FRU_BRDINFO_MFT_TLV_STRT, skip_len = 0;
    uint32_t count = 1;
    uint8_t  ret = 0;
    char path[100] = {0}, data[100] = {0};
    char tmp[100] = {0};

    switch (id)
    {
        case CHASSIS_FAN1_ID:
            strcpy(path, FAN1_EEPROM);
            break;
        case CHASSIS_FAN2_ID:
            strcpy(path, FAN2_EEPROM);
            break;
        case CHASSIS_FAN3_ID:
            strcpy(path, FAN3_EEPROM);
            break;
        case CHASSIS_FAN4_ID:
            strcpy(path, FAN4_EEPROM);
            break;
    }

    ret = read_eeprom(path , offset, data, &count);
    if (ret < 0)
        return ret;

    skip_len = data[0] & TLV_LEN_MASK; // skip Board Manufacturer bytes
    offset += skip_len + 1 ;
    count = 1;
    ret = read_eeprom(path , offset, data, &count);
    if (ret < 0)
        return ret;

    skip_len = data[0] & TLV_LEN_MASK; // skip Board Product Name
    offset += skip_len + 1;
    count = 1;
    ret = read_eeprom(path , offset, data, &count);
    if (ret < 0)
        return ret;

    count = data[0] & TLV_LEN_MASK; // Board Serial Number bytes length
    offset++;
    if (count > sizeof(data)) {
        printf("read count %d is larger than buffer size %ld\n", count, sizeof(data));
        return -1;
    }
    ret = read_eeprom(path , offset, data, &count);
    if (ret < 0)
        return ret;

    strcpy(serial, data);
    memset(data, 0, sizeof(data));
    offset += count;
    count = 1;
    ret = read_eeprom(path , offset, data, &count);
    if (ret < 0)
        return ret;

    count = data[0] & TLV_LEN_MASK; // Board Part Number bytes length
    offset++;
    if (count > sizeof(data)) {
        printf("read count %d is larger than buffer size %ld\n", count, sizeof(data));
        return -1;
    }
    ret = read_eeprom(path , offset, data, &count);
    if (ret < 0)
        return ret;

    strcpy(model, data);

    sprintf(tmp, "%sfan%d_direction", CHASSIS_FAN_PATH, id);
    get_sysnode_value(tmp, isfanb2f);
    /* 0-B2F,1-F2B*/
    *isfanb2f = (*isfanb2f) ?  ONLP_FAN_STATUS_B2F : ONLP_FAN_STATUS_F2B;

    return 0;
}

int get_fan_airflow(void)
{
    int ret;
    int air_flow;

    ret = get_sysnode_value(CHASSIS_FAN_DIRECTION(4), &air_flow); //Choose No.4 fan to get real air_flow
    if (ret < 0) 
        return ret;

    if(air_flow)
        return FAN_DIR_B2F;
    else
        return FAN_DIR_F2B;
}

int get_sensor_info_wobmc(int id, int *temp, int *warn, int *error, int *shutdown)
{
    int ret = 0, fail = 0;
    int air_flow;
     *temp = *warn = *error = *shutdown = 0;

    if ((NULL == temp) || (NULL == warn) || (NULL == error) || (NULL == shutdown)) {
        printf("%s null pointer!\n", __FUNCTION__);
        return -1;
    }
    air_flow = get_fan_airflow();
    switch (id)
    {
        case Base_Temp_U5_ID:
            if(air_flow == FAN_DIR_B2F){
                *warn = Base_Temp_U5_B2F_HI_WARN;
                *error = Base_Temp_U5_B2F_HI_ERR;
                *shutdown = Base_Temp_U5_B2F_HI_SHUTDOWN;
            }
            else{
                *warn = Base_Temp_U5_F2B_HI_WARN;
                *error = Base_Temp_U5_F2B_HI_ERR;
                *shutdown = Base_Temp_U5_F2B_HI_SHUTDOWN;
            }
            ret = get_sysnode_value(Base_Temp_U5, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case Base_Temp_U56_ID:
            if(air_flow == FAN_DIR_B2F){
                *warn = Base_Temp_U56_B2F_HI_WARN;
                *error = Base_Temp_U56_B2F_HI_ERR;
                *shutdown = Base_Temp_U56_B2F_HI_SHUTDOWN;
            }
            else{
                *warn = Base_Temp_U56_F2B_HI_WARN;
                *error = Base_Temp_U56_F2B_HI_ERR;
                *shutdown = Base_Temp_U56_F2B_HI_SHUTDOWN;
            }
            ret = get_sysnode_value(Base_Temp_U56, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case Switch_Temp_U31_ID:
            if(air_flow == FAN_DIR_B2F){
                *warn = Switch_Temp_U31_B2F_HI_WARN;
                *error = Switch_Temp_U31_B2F_HI_ERR;
                *shutdown = Switch_Temp_U31_B2F_HI_SHUTDOWN;
            }
            else{
                *warn = Switch_Temp_U31_F2B_HI_WARN;
                *error = Switch_Temp_U31_F2B_HI_ERR;
                *shutdown = Switch_Temp_U31_F2B_HI_SHUTDOWN;
            }
            ret = get_sysnode_value(Switch_Temp_U31, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case Switch_Temp_U30_ID:
                *warn = Switch_Temp_U30_HI_WARN;
                *error = Switch_Temp_U30_HI_ERR;
                *shutdown = Switch_Temp_U30_HI_SHUTDOWN;
            ret = get_sysnode_value(Switch_Temp_U30, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case Switch_Temp_U29_ID:
            if(air_flow == FAN_DIR_B2F){
                *warn = Switch_Temp_U29_B2F_HI_WARN;
                *error = Switch_Temp_U29_B2F_HI_ERR;
                *shutdown = Switch_Temp_U29_B2F_HI_SHUTDOWN;
            }
            else{
                *warn = Switch_Temp_U29_F2B_HI_WARN;
                *error = Switch_Temp_U29_F2B_HI_ERR;
                *shutdown = Switch_Temp_U29_F2B_HI_SHUTDOWN;
            }
            ret = get_sysnode_value(Switch_Temp_U29, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;	
        case Switch_Temp_U28_ID:
            if(air_flow == FAN_DIR_B2F){
                *warn = Switch_Temp_U28_B2F_HI_WARN;
                *error = Switch_Temp_U28_B2F_HI_ERR;
                *shutdown = Switch_Temp_U28_B2F_HI_SHUTDOWN;
            }
            else{
                *warn = Switch_Temp_U28_F2B_HI_WARN;
                *error = Switch_Temp_U28_F2B_HI_ERR;
                *shutdown = Switch_Temp_U28_F2B_HI_SHUTDOWN;
            }
            ret = get_sysnode_value(Switch_Temp_U28, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;	
        case CPU_Temp_ID:
            *warn = CPU_Temp_HI_WARN;
            *error = CPU_Temp_HI_ERR;
            *shutdown = CPU_Temp_HI_SHUTDOWN;
            ret = get_sysnode_value(CPU_Temp, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case TEMP_DIMMA0_ID:
            *warn = DIMMA0_THERMAL_HI_WARN;
            *error = DIMMA0_THERMAL_HI_ERR;
            *shutdown = DIMMA0_THERMAL_HI_SHUTDOWN;
            ret = get_sysnode_value(TEMP_DIMMA0, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case TEMP_DIMMB0_ID:
            *warn = DIMMB0_THERMAL_HI_WARN;
            *error = DIMMB0_THERMAL_HI_ERR;
            *shutdown = DIMMB0_THERMAL_HI_SHUTDOWN;
            ret = get_sysnode_value(TEMP_DIMMB0, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case VDD_CORE_Temp_ID:
            *warn = VDD_CORE_Temp_HI_WARN;
            *error = VDD_CORE_Temp_HI_ERR;
            *shutdown = VDD_CORE_Temp_HI_SHUTDOWN;
            ret = get_sysnode_value(VDD_CORE_Temp, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case VDD_ANLG_Temp_ID:
            *warn = VDD_ANLG_Temp_HI_WARN;
            *error = VDD_ANLG_Temp_HI_ERR;
            *shutdown = VDD_ANLG_Temp_HI_SHUTDOWN;
            ret = get_sysnode_value(VDD_ANLG_Temp, temp);
            if(ret < 0 || *temp == 0)
                fail++;
            break;
        case PSU1_Temp1_ID:
            if (psu1_src == MAIN_SRC) {
                *warn = PSU_Temp1_HI_WARN;
                *error = PSU_Temp1_HI_ERR;
                *shutdown = PSU_Temp1_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU1_Temp1, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else if (psu1_src == SECD_SRC) {
                *warn = PSU_GSP_Temp1_HI_WARN;
                *error = PSU_GSP_Temp1_HI_ERR;
                *shutdown = PSU_GSP_Temp1_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU1_Temp1, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else {
                *warn = UNDEFINE;
                *error = UNDEFINE;
                *shutdown = UNDEFINE;
                ret = get_sysnode_value(PSU1_Temp1, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
        case PSU1_Temp2_ID:
            if (psu1_src == MAIN_SRC) {
                *warn = PSU_Temp2_HI_WARN;
                *error = PSU_Temp2_HI_ERR;
                *shutdown = PSU_Temp2_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU1_Temp2, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else if (psu1_src == SECD_SRC) {
                *warn = PSU_GSP_Temp2_HI_WARN;
                *error = PSU_GSP_Temp2_HI_ERR;
                *shutdown = PSU_GSP_Temp2_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU1_Temp2, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else {
                *warn = UNDEFINE;
                *error = UNDEFINE;
                *shutdown = UNDEFINE;
                ret = get_sysnode_value(PSU1_Temp2, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
        case PSU2_Temp1_ID:
            if (psu2_src == MAIN_SRC) {
                *warn = PSU_Temp1_HI_WARN;
                *error = PSU_Temp1_HI_ERR;
                *shutdown = PSU_Temp1_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU2_Temp1, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else if (psu2_src == SECD_SRC) {
                *warn = PSU_GSP_Temp1_HI_WARN;
                *error = PSU_GSP_Temp1_HI_ERR;
                *shutdown = PSU_GSP_Temp1_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU2_Temp1, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else {
                *warn = UNDEFINE;
                *error = UNDEFINE;
                *shutdown = UNDEFINE;
                ret = get_sysnode_value(PSU2_Temp1, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
        case PSU2_Temp2_ID:
            if (psu2_src == MAIN_SRC) {
                *warn = PSU_Temp2_HI_WARN;
                *error = PSU_Temp2_HI_ERR;
                *shutdown = PSU_Temp2_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU2_Temp2, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else if (psu2_src == SECD_SRC) {
                *warn = PSU_GSP_Temp2_HI_WARN;
                *error = PSU_GSP_Temp2_HI_ERR;
                *shutdown = PSU_GSP_Temp2_HI_SHUTDOWN;
                ret = get_sysnode_value(PSU2_Temp2, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
            else {
                *warn = UNDEFINE;
                *error = UNDEFINE;
                *shutdown = UNDEFINE;
                ret = get_sysnode_value(PSU2_Temp2, temp);
                if(ret < 0 || *temp == 0)
                    fail++;
                break;
            }
        default:
            break;
    }

    return fail;
}

int get_fan_speed_wobmc(int id,int *per, int *rpm)
{
    int ret = 0, fail = 0;;

    *rpm = *per = 0;

    if ((NULL == per) || (NULL == rpm)) {
        printf("%s null pointer!\n", __FUNCTION__);
        return -1;
    }
    switch (id)
    {
        case CHASSIS_FAN1_ID:
            ret = get_sysnode_value(CHASSIS_FAN_SPEED(1), rpm);
            if (ret < 0 || *rpm == 0)
                fail++;
            else
                *per = *rpm * 100 / CHASSIS_FAN_REAR_MAX_RPM;
            break;
        case CHASSIS_FAN2_ID:
            ret = get_sysnode_value(CHASSIS_FAN_SPEED(2), rpm);
            if (ret < 0 || *rpm == 0)
                fail++;
            else
                *per = *rpm * 100 / CHASSIS_FAN_REAR_MAX_RPM;
            break;
        case CHASSIS_FAN3_ID:
            ret = get_sysnode_value(CHASSIS_FAN_SPEED(3), rpm);
            if (ret < 0 || *rpm == 0)
                fail++;
            else
                *per = *rpm * 100 / CHASSIS_FAN_REAR_MAX_RPM;
            break;
        case CHASSIS_FAN4_ID:
            ret = get_sysnode_value(CHASSIS_FAN_SPEED(4), rpm);
            if (ret < 0 || *rpm == 0)
                fail++;
            else
                *per = *rpm * 100 / CHASSIS_FAN_REAR_MAX_RPM;
            break;
        case PSU1_FAN_ID:
            ret = get_sysnode_value(PSU1_FAN, rpm);
            if (ret < 0 || *rpm == 0)
                fail++;
            else
                *per = *rpm * 100 / PSU_FAN_MAX_RPM;
            break;
        case PSU2_FAN_ID:
            ret = get_sysnode_value(PSU2_FAN, rpm);
            if (ret < 0 || *rpm == 0)
                fail++;
            else
                *per = *rpm * 100 / PSU_FAN_MAX_RPM;
            break;
        default:
            break;
    }

    return fail;
}

