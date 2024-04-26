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
#include <linux/i2c.h>
#include <onlplib/i2c.h>
#include "platform_common.h"
#include "platform_wbmc.h"
#include "platform_wobmc.h"


static const struct led_reg_mapper led_mapper[LED_COUNT + 1] = {
    {},
    {"LED_SYSTEM", LED_SYSTEM_ID, LED_SYSTEM_REG},
    {"LED_ALARM",  LED_ALARM_ID,  LED_ALARM_REG},
    {"LED_PSU",    LED_PSU_ID,    LED_PSU_REG},
    {"LED_FAN",    LED_FAN_ID,    LED_FAN_REG},
    {"LED_FAN1",   LED_FAN1_ID,   LED_FAN1_REG},
    {"LED_FAN2",   LED_FAN2_ID,   LED_FAN2_REG},
    {"LED_FAN3",   LED_FAN3_ID,   LED_FAN3_REG},
    {"LED_FAN4",   LED_FAN4_ID,   LED_FAN4_REG}
};

const struct psu_reg_bit_mapper psu_mapper [PSU_COUNT + 1] = {
    {},
    {PSU_STA_REG, 1, 3, 5},
    {PSU_STA_REG, 0, 2, 4},
};

char* trim (char *s)
{
    int i;

    while (isspace (*s)) s++;   // skip left side white spaces
    for (i = strlen (s) - 1; (isspace (s[i])); i--) ;   // skip right side white spaces
    s[i + 1] = '\0';

    return s;
}

void array_trim(char *strIn, char *strOut)
{
    int i, j;

    i = 0;
    j = strlen(strIn) - 1;

    while(strIn[i] == ' ') ++i;
    while(strIn[j] == ' ') --j;

    /* in case all characters are spaces */
    if (-1 != j) {
        strncpy(strOut, strIn + i , j - i + 1);
        strOut[j - i + 1] = '\0';
    }
}

uint16_t check_bmc_status(void)
{
    uint8_t status = -1;

    status = read_register(CARD_PRESENT_STATE_REG,SYS_CPLD_PATH) & 0x01;
    if(status == 0)  //present
        return PRESENT;
    else if(status == 1)  //absent
        return ABSENT;

    return -1;
}


uint8_t read_register(uint16_t dev_reg, char *path)
{
    char command[256];
    uint8_t value;
    int fd = 0;
    int ret = 0;

    sprintf(command, "%sdump", path);
    fd = open(command, O_RDONLY);
    if (fd < 0) {
        printf("ERROR: Fail to open the file: %s \n", command);
        return ERR;
    }
    lseek(fd, dev_reg, SEEK_SET);   //Move to offset 'dev_reg' bytes from the beginning of the file
    ret = read(fd, &value, sizeof(value));
    if (ret < 0) {
        printf("ERROR: Fail to read register: 0x%x \n", value);
        close(fd);
        return ERR;
    }
    close(fd);

    return value;
}

int syscpld_setting(uint32_t reg, uint8_t val)
{
    char data[30] = {0};
    char path[100] = {0};
    int fd, rc = 0;

    sprintf(path, "%s%s",SYS_CPLD_PATH, "setreg");
    sprintf(data, "0x%x 0x%x",reg, val);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Fail to open the file: %s \n", path);
        return ERR;
    }

    rc = write(fd, data, strlen(data));
    if (rc != strlen(data)) {
        printf("Write failed. count=%lu, rc=%d \n", strlen(data), rc);
        close(fd);
        return ERR;
    }

    close(fd);

    return OK;
}

uint8_t get_led_status(int id)
{
    uint8_t ret = 0xFF;
    int result = 0;
    uint16_t led_stat_reg;
    uint16_t bmc_flag = -1;

    if (id > LED_COUNT || id < 0){
        return ERR;
    }

    bmc_flag = check_bmc_status();
    if(bmc_flag == -1)
        return bmc_flag;

    led_stat_reg = led_mapper[id].dev_reg; 

    if (id <= LED_COUNT) {
        result = read_register(led_stat_reg, SYS_CPLD_PATH);
        ret = result;
        }

    return ret;
}

uint8_t get_psu_status(int id)
{
    uint8_t ret = 0xFF;
    uint16_t psu_stat_reg;

    if (id <= (PSU_COUNT))
    {
        uint8_t result = 0;
        psu_stat_reg = psu_mapper[id].sta_reg;
        result = read_register(psu_stat_reg, SYS_CPLD_PATH);
        ret = result;
    }

    return ret;
}

int check_psu_bom(int id, char *model)
{
    uint8_t psu_status = 0, present_status = 0;
    int psu_src = 0;

    present_status = (psu_status >> psu_mapper[id].bit_present) & 0x01;

    if (0 == present_status) {
        if ((0 == strcmp(MAIN_SRC_PSU_MODEL_F2B, model)) || (0 == strcmp(MAIN_SRC_PSU_MODEL_B2F, model))) {
            psu_src = MAIN_SRC;
            return psu_src;
            }
        else if ((0 == strcmp(SECD_SRC_PSU_MODEL_F2B, model)) || (0 == strcmp(SECD_SRC_PSU_MODEL_B2F, model))) {
            psu_src = SECD_SRC;
            return psu_src;
            }
        else {
            psu_src = UNKNOWN_SRC;
            printf("!!! PSU%d is an unknown source and is not compatible with onlpd !!!\n\
!!! Please replace this PSU first !!!\n", id);
            return psu_src;
            }
    }

    return psu_src;
}

int get_psu_info(int id, int *mvin, int *mvout, int *mpin, int *mpout, int *miin, int *miout)
{
    uint16_t bmc_flag = -1;
    int ret = -1;

    bmc_flag = check_bmc_status();
    if(bmc_flag == -1)
        return bmc_flag;

    if(bmc_flag)
        ret = get_psu_info_wobmc(id, mvin, mvout, mpin, mpout, miin, miout);
    else
        ret = get_psu_info_wbmc(id, mvin, mvout, mpin, mpout, miin, miout);

    return ret;
}

int get_psu_model_sn(int id, char *model, char *serial_number)
{
    uint16_t bmc_flag = -1;
    int ret = -1;

    bmc_flag = check_bmc_status();
    if(bmc_flag == -1)
        return bmc_flag;

    if(bmc_flag)
        ret = get_psu_model_sn_wobmc(id, model, serial_number);
    else 
        ret = get_psu_model_sn_wbmc(id, model, serial_number);

    return ret;
}

int get_fan_info(int id, char *model, char *serial, int *isfanb2f)
{
    uint16_t bmc_flag = -1;
    int ret = -1;

    bmc_flag = check_bmc_status();
    if(bmc_flag == -1)
        return bmc_flag;

    if(bmc_flag)
        ret = get_fan_info_wobmc(id, model, serial, isfanb2f);
    else
        ret = get_fan_info_wbmc(id, model, serial, isfanb2f);

    return ret;
}

int get_sensor_info(int id, int *temp, int *warn, int *error, int *shutdown)
{
    uint16_t bmc_flag = -1;
    int ret = -1;

    bmc_flag = check_bmc_status();
    if(bmc_flag == -1)
        return bmc_flag;

    if(bmc_flag)
        ret = get_sensor_info_wobmc(id, temp, warn, error, shutdown);
    else
        ret = get_sensor_info_wbmc(id, temp, warn, error, shutdown);

    return ret;

}

int get_fan_speed(int id,int *per, int *rpm)
{
    uint16_t bmc_flag = -1;
    int ret = -1;

    bmc_flag = check_bmc_status();
    if(bmc_flag == -1)
        return bmc_flag;

    if(bmc_flag)
        ret = get_fan_speed_wobmc(id, per, rpm);
    else 
        ret = get_fan_speed_wbmc(id, per, rpm);

    return ret;
}

int read_device_node_binary(char *filename, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0))
        return -1;

    if ((fd = open(filename, O_RDONLY)) == -1)
        return -1;

    if ((len = read(fd, buffer, buf_size)) < 0)
    {
        close(fd);
        return -1;
    }

    if ((close(fd) == -1))
        return -1;

    if ((len > buf_size) || (data_len != 0 && len != data_len))
        return -1;

    return 0;
}

int read_device_node_string(char *filename, char *buffer, int buf_size, int data_len)
{
    int ret;

    if (data_len >= buf_size)
    {
        return -1;
    }

    ret = read_device_node_binary(filename, buffer, buf_size - 1, data_len);
    if (ret == 0)
    {
        buffer[buf_size - 1] = '\0';
    }

    return ret;
}
