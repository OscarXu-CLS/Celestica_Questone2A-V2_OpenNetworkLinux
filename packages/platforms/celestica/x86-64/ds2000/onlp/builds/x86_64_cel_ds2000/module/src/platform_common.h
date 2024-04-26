#ifndef _PLATFORM_DS2000_COMMON_H_
#define _PLATFORM_DS2000_COMMON_H_
#include <stdint.h>
#include <pthread.h>

#define PREFIX_PATH_LEN 100
#define CARD_PRESENT_STATE_REG   0x08
#define PRESENT 0
#define ABSENT  1

#define ERR        -1
#define OK          0
#define FAULT       2
#define TEMP_ERR    -99


//Number of devices
#define FAN_COUNT             6
#define CHASSIS_FAN_COUNT     4
#define PSU_FAN_COUNT         2
#define THERMAL_COUNT         15
#define CHASSIS_THERMAL_COUNT 11
#define PSU_COUNT             2
#define SFP_COUNT             48
#define QSFP_COUNT            8
#define LED_COUNT             8

//Device registers
#define PSU_STA_REG      0x60
#define LED_PSU_REG      0x61
#define LED_PSU_GRN      0x4e
#define LED_PSU_AMB      0x8d
#define LED_SYSTEM_REG   0x62
#define LED_ALARM_REG    0x63
#define LED_FAN_REG      0x65
#define LED_FAN_GRN      0x4e
#define LED_FAN_AMB      0x8d
#define LED_FAN_OFF      0xcf
#define LED_FAN1_REG     0xb3
#define LED_FAN2_REG     0xb9
#define LED_FAN3_REG     0xc5
#define LED_FAN4_REG     0xcB
#define THERMAL_REG      0x76

#define FAN_LED_AMB      0x01
#define FAN_LED_GRN      0x02
#define FAN_LED_OFF      0x03


#define LPC_COME_BTNCTRL_REG   0xA124
#define LPC_SWITCH_PWCTRL_REG  0xA126
#define LPC_LED_PSU_REG        0xA161
#define LPC_LED_FAN_REG        0xA165
#define LPC_FAN1_LED_REG       0xA1B3
#define LPC_FAN2_LED_REG       0xA1B9
#define LPC_FAN3_LED_REG       0xA1C5
#define LPC_FAN4_LED_REG       0xA1CB


#define COME_BTNOFF_HOLD       0x00
#define COME_BTNOFF_RELEASE    0x01
#define COME_BTNOFF_HOLD_SEC   8

#define SWITCH_OFF             0x01
#define SWITCH_ON              0x00

//Chassis and PSU FANs maximum RPM
#define CHASSIS_FAN_REAR_MAX_RPM  20300
#define CHASSIS_FAN_FRONT_MAX_RPM  23300
#define PSU_FAN_MAX_RPM      18000

#define CHASIS_FAN_TOLERANT_PER 25

//Offset
#define SFP_BUS_OFFSET 12

//LED mode
#define LED_SYSTEM_BOTH    0
#define LED_SYSTEM_GREEN   1
#define LED_SYSTEM_YELLOW  2
#define LED_SYSTEM_OFF     3
#define LED_SYSTEM_4_HZ    2
#define LED_SYSTEM_1_HZ    1

//PSU module
#define MAIN_SRC_PSU_MODEL_F2B  "FSP550-20FM"
#define MAIN_SRC_PSU_MODEL_B2F  "FSP550-29FM"
#define SECD_SRC_PSU_MODEL_F2B  "G1251-0550WNA"
#define SECD_SRC_PSU_MODEL_B2F  "G1251-0550WRA"

#define MAIN_SRC       1
#define SECD_SRC       2
#define UNKNOWN_SRC   -1


enum led_id {
    LED_SYSTEM_ID = 1,
    LED_ALARM_ID,
    LED_PSU_ID,
    LED_FAN_ID,
    LED_FAN1_ID,
    LED_FAN2_ID,
    LED_FAN3_ID,
    LED_FAN4_ID,
};

//Thermal
enum thermal_id{
    Base_Temp_U5_ID = 1,
    Base_Temp_U56_ID,
    Switch_Temp_U31_ID,
    Switch_Temp_U30_ID,
    Switch_Temp_U29_ID,
    Switch_Temp_U28_ID,
    CPU_Temp_ID,
    TEMP_DIMMA0_ID,
    TEMP_DIMMB0_ID,
    VDD_CORE_Temp_ID,
    VDD_ANLG_Temp_ID,
    PSU1_Temp1_ID,
    PSU1_Temp2_ID,
    PSU2_Temp1_ID,
    PSU2_Temp2_ID,
};

//COMMON USE
#define NELEMS(x)             (sizeof(x) / sizeof((x)[0]))
#define DEBUG_MODE 0

#ifdef DEBUG_MODE
#define DEBUG_PRINT(fmt, args...)    fprintf(stderr, fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

enum psu_id {
    PSU1_ID = 1,
    PSU2_ID,
};

enum fan_id{
    CHASSIS_FAN1_ID = 1,
    CHASSIS_FAN2_ID,
    CHASSIS_FAN3_ID,
    CHASSIS_FAN4_ID,
    PSU1_FAN_ID,
    PSU2_FAN_ID,
};


#define NUM_OF_CPLD 1

struct fan_config_p{
    uint16_t pwm_reg;
    uint16_t ctrl_sta_reg;
    uint16_t rear_spd_reg;
    uint16_t front_spd_reg;
};

struct led_reg_mapper{
    char *name;
    uint16_t device;
    uint16_t dev_reg;
};

struct psu_reg_bit_mapper{
    uint16_t sta_reg;
    uint8_t bit_pwrok_sta;
    uint8_t bit_present;
    uint8_t bit_alt_sta;
};

#define SYS_CPLD_PATH "/sys/devices/platform/sys_cpld/"
#define PLATFORM_PATH "/sys/class/SFF"
#define I2C_DEVICE_PATH "/sys/bus/i2c/devices/"
#define SYS_FPGA_PATH "/sys/devices/platform/fpga-sys/"
#define PREFIX_PATH_ON_SYS_EEPROM "/sys/bus/i2c/devices/i2c-1/1-0056/eeprom"

char* trim (char *s);
void array_trim(char *strIn, char *strOut);
uint16_t check_bmc_status(void);
uint8_t read_register(uint16_t dev_reg,char *path);
int syscpld_setting(uint32_t reg, uint8_t val);
uint8_t get_led_status(int id);
uint8_t get_psu_status(int id);
int check_psu_bom(int id, char *model);
int get_psu_info(int id, int *mvin, int *mvout, int *mpin, int *mpout, int *miin, int *miout);
int get_psu_model_sn(int id, char *model, char *serial_number);
int get_fan_info(int id, char *model, char *serial, int *isfanb2f);
int get_sensor_info(int id, int *temp, int *warn, int *error, int *shutdown);
int get_fan_speed(int id,int *per, int *rpm);
int read_device_node_binary(char *filename, char *buffer, int buf_size, int data_len);
int read_device_node_string(char *filename, char *buffer, int buf_size, int data_len);
int create_cache();
int is_cache_exist();


#endif /* _PLATFORM_DS2000_COMMON_H_ */
