#ifndef _PLATFORM_DS2000_WOBMC_H_
#define _PLATFORM_DS2000_WOBMC_H_
#include <stdint.h>
#include <onlp/platformi/fani.h>
#include "platform_common.h"


#define FAN1_EEPROM                     "/sys/bus/i2c/devices/i2c-81/81-0050/eeprom"
#define FAN2_EEPROM                     "/sys/bus/i2c/devices/i2c-80/80-0050/eeprom"
#define FAN3_EEPROM                     "/sys/bus/i2c/devices/i2c-78/78-0050/eeprom"
#define FAN4_EEPROM                     "/sys/bus/i2c/devices/i2c-77/77-0050/eeprom"



#define BITS_PER_LONG_LONG 64

#define GENMASK_ULL(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & \
	 (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

#define FRU_COMM_HDR_LEN                8
#define FRU_BRDINFO_MFT_TLV_STRT        6   //Board Manufacturer byte offset in "Board Info Area Format" field. 
#define TLV_TYPE_MASK                   GENMASK_ULL(7,6)
#define TLV_LEN_MASK                    GENMASK_ULL(5,0)

//Fan
#define FAN_CPLD_I2C_BUS_STR          "7"
#define FAN_CPLD_I2C_ADDR_STR         "000d"

#define PSU1_I2C_BUS_STR               "69"
#define PSU2_I2C_BUS_STR               "70"
#define PSU1_I2C_ADDR_STR             "005a"
#define PSU2_I2C_ADDR_STR             "005b"

#define PSU1_I2C_BUS                   69
#define PSU2_I2C_BUS                   70
#define PSU1_I2C_ADDR                 0x5a
#define PSU2_I2C_ADDR                 0x5b

#define CHASSIS_FAN_PATH              "/sys/bus/i2c/devices/i2c-"FAN_CPLD_I2C_BUS_STR"/"FAN_CPLD_I2C_BUS_STR"-"FAN_CPLD_I2C_ADDR_STR"/"
#define CHASSIS_FAN_DIRECTION(x)      "/sys/bus/i2c/devices/i2c-"FAN_CPLD_I2C_BUS_STR"/"FAN_CPLD_I2C_BUS_STR"-"FAN_CPLD_I2C_ADDR_STR"/fan"#x"_direction"
#define CHASSIS_FAN_FAULT(x)          "/sys/bus/i2c/devices/i2c-"FAN_CPLD_I2C_BUS_STR"/"FAN_CPLD_I2C_BUS_STR"-"FAN_CPLD_I2C_ADDR_STR"/fan"#x"_fault"
#define CHASSIS_FAN_SPEED(x)          "/sys/bus/i2c/devices/i2c-"FAN_CPLD_I2C_BUS_STR"/"FAN_CPLD_I2C_BUS_STR"-"FAN_CPLD_I2C_ADDR_STR"/fan"#x"_rear_speed_rpm"
#define CHASSIS_FAN_LED(x)            "/sys/bus/i2c/devices/i2c-"FAN_CPLD_I2C_BUS_STR"/"FAN_CPLD_I2C_BUS_STR"-"FAN_CPLD_I2C_ADDR_STR"/fan"#x"_led"
#define CHASSIS_FAN_PRESENT(x)        "/sys/bus/i2c/devices/i2c-"FAN_CPLD_I2C_BUS_STR"/"FAN_CPLD_I2C_BUS_STR"-"FAN_CPLD_I2C_ADDR_STR"/fan"#x"_present"
#define CHASSIS_FAN_PWM(x)            "/sys/bus/i2c/devices/i2c-"FAN_CPLD_I2C_BUS_STR"/"FAN_CPLD_I2C_BUS_STR"-"FAN_CPLD_I2C_ADDR_STR"/pwm"#x

#define PSU1_FAN                      "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/fan1_input"
#define PSU2_FAN                      "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/fan1_input"

#define FAN_DIR_F2B                   0
#define FAN_DIR_B2F                   1

//PSU sensor
#define PSU1_CIN  "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/curr1_input"
#define PSU1_VIN  "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/in1_input"
#define PSU1_PIN  "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/power1_input"
#define PSU1_COUT "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/curr2_input"
#define PSU1_VOUT "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/in2_input"
#define PSU1_POUT "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/power2_input"
#define PSU2_CIN  "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/curr1_input"
#define PSU2_VIN  "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/in1_input"
#define PSU2_PIN  "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/power1_input"
#define PSU2_COUT "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/curr2_input"
#define PSU2_VOUT "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/in2_input"
#define PSU2_POUT "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/power2_input"


#define PMBUS_MODEL_REG 0x9a
#define PMBUS_SERIAL_REG 0x9e


#define Base_Temp_U5      "/sys/bus/i2c/devices/i2c-11/11-004d/hwmon/hwmon*/temp1_input"
#define Base_Temp_U56     "/sys/bus/i2c/devices/i2c-11/11-004e/hwmon/hwmon*/temp1_input"
#define Switch_Temp_U31   "/sys/bus/i2c/devices/i2c-11/11-004c/hwmon/hwmon*/temp1_input"
#define Switch_Temp_U30   "/sys/bus/i2c/devices/i2c-11/11-0049/hwmon/hwmon*/temp1_input"
#define Switch_Temp_U29   "/sys/bus/i2c/devices/i2c-11/11-004a/hwmon/hwmon*/temp1_input"
#define Switch_Temp_U28   "/sys/bus/i2c/devices/i2c-11/11-004b/hwmon/hwmon*/temp1_input"
#define CPU_Temp          "/sys/devices/platform/coretemp.0/hwmon/hwmon*/temp1_input"
#define TEMP_DIMMA0       "/sys/bus/i2c/devices/0-0018/hwmon/hwmon*/temp1_input"
#define TEMP_DIMMB0       "/sys/bus/i2c/devices/0-001a/hwmon/hwmon*/temp1_input"
#define VDD_CORE_Temp     "/sys/bus/i2c/devices/i2c-10/10-007a/hwmon/hwmon*/temp1_input"
#define VDD_ANLG_Temp     "/sys/bus/i2c/devices/i2c-10/10-0070/hwmon/hwmon*/temp1_input"
#define PSU1_Temp1        "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/temp1_input"
#define PSU1_Temp2        "/sys/bus/i2c/devices/i2c-"PSU1_I2C_BUS_STR"/"PSU1_I2C_BUS_STR"-"PSU1_I2C_ADDR_STR"/hwmon/hwmon*/temp2_input"
#define PSU2_Temp1        "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/temp1_input"
#define PSU2_Temp2        "/sys/bus/i2c/devices/i2c-"PSU2_I2C_BUS_STR"/"PSU2_I2C_BUS_STR"-"PSU2_I2C_ADDR_STR"/hwmon/hwmon*/temp2_input"

/* define by Thermal team*/
#define U5_F2B_MAX    68000
#define U56_F2B_MAX   68000
#define U30_MAX       65000
#define U29_B2F_MAX   60000
#define U28_B2F_MAX   60000

/* Warning */
#define Base_Temp_U5_B2F_HI_WARN         55000
#define Base_Temp_U5_F2B_HI_WARN         U5_F2B_MAX
#define Base_Temp_U56_B2F_HI_WARN        55000
#define Base_Temp_U56_F2B_HI_WARN        U56_F2B_MAX
#define Base_Temp_U5_B2F_LC_WARN         -5000
#define Base_Temp_U56_B2F_LC_WARN        -5000
#define Switch_Temp_U28_F2B_LC_WARN      -5000
#define Switch_Temp_U29_F2B_LC_WARN      -5000
#define Switch_Temp_U31_B2F_HI_WARN      80000
#define Switch_Temp_U31_F2B_HI_WARN      78000
#define Switch_Temp_U30_HI_WARN          U30_MAX
#define Switch_Temp_U29_B2F_HI_WARN      U29_B2F_MAX
#define Switch_Temp_U29_F2B_HI_WARN      58000
#define Switch_Temp_U28_B2F_HI_WARN      U28_B2F_MAX
#define Switch_Temp_U28_F2B_HI_WARN      58000
#define CPU_Temp_HI_WARN                 90000
#define DIMMA0_THERMAL_HI_WARN           84000
#define DIMMB0_THERMAL_HI_WARN           84000
#define VDD_CORE_Temp_HI_WARN            120000
#define VDD_ANLG_Temp_HI_WARN            120000
#define PSU_Temp1_HI_WARN                65000
#define PSU_Temp2_HI_WARN                78000
#define PSU_GSP_Temp1_HI_WARN            60000
#define PSU_GSP_Temp2_HI_WARN            108000



/* Error */
#define Base_Temp_U5_B2F_HI_ERR          58000
#define Base_Temp_U5_F2B_HI_ERR          U5_F2B_MAX
#define Base_Temp_U56_B2F_HI_ERR         58000
#define Base_Temp_U56_F2B_HI_ERR         U56_F2B_MAX
#define Switch_Temp_U31_B2F_HI_ERR       82000
#define Switch_Temp_U31_F2B_HI_ERR       78000
#define Switch_Temp_U30_HI_ERR           U30_MAX
#define Switch_Temp_U29_B2F_HI_ERR       U29_B2F_MAX
#define Switch_Temp_U29_F2B_HI_ERR       63000
#define Switch_Temp_U28_B2F_HI_ERR       U28_B2F_MAX
#define Switch_Temp_U28_F2B_HI_ERR       63000
#define CPU_Temp_HI_ERR                  90000
#define DIMMA0_THERMAL_HI_ERR            84000
#define DIMMB0_THERMAL_HI_ERR            84000
#define VDD_CORE_Temp_HI_ERR             120000
#define VDD_ANLG_Temp_HI_ERR             120000
#define PSU_Temp1_HI_ERR                 65000
#define PSU_Temp2_HI_ERR                 78000
#define PSU_GSP_Temp1_HI_ERR             60000
#define PSU_GSP_Temp2_HI_ERR             108000



/* Shutdown */
#define Base_Temp_U5_B2F_HI_SHUTDOWN     58000
#define Base_Temp_U5_F2B_HI_SHUTDOWN     U5_F2B_MAX
#define Base_Temp_U56_B2F_HI_SHUTDOWN    58000
#define Base_Temp_U56_F2B_HI_SHUTDOWN    U56_F2B_MAX
#define Switch_Temp_U31_B2F_HI_SHUTDOWN  85000
#define Switch_Temp_U31_F2B_HI_SHUTDOWN  80000
#define Switch_Temp_U30_HI_SHUTDOWN      U30_MAX
#define Switch_Temp_U29_B2F_HI_SHUTDOWN  U29_B2F_MAX
#define Switch_Temp_U29_F2B_HI_SHUTDOWN  63000
#define Switch_Temp_U28_B2F_HI_SHUTDOWN  U28_B2F_MAX
#define Switch_Temp_U28_F2B_HI_SHUTDOWN  63000
#define CPU_Temp_HI_SHUTDOWN             90000
#define DIMMA0_THERMAL_HI_SHUTDOWN       84000
#define DIMMB0_THERMAL_HI_SHUTDOWN       84000
#define VDD_CORE_Temp_HI_SHUTDOWN        120000
#define VDD_ANLG_Temp_HI_SHUTDOWN        120000
#define PSU_Temp1_HI_SHUTDOWN            65000
#define PSU_Temp2_HI_SHUTDOWN            78000
#define PSU_GSP_Temp1_HI_SHUTDOWN        60000
#define PSU_GSP_Temp2_HI_SHUTDOWN        108000

#define UNDEFINE                         0

int read_smbus_block(int bus, int address, int bank, unsigned char *cblock);
int read_eeprom(char* path, int offset, char* data, unsigned int *count);
int get_sysnode_value(const char *path, void *dat);
int set_sysnode_value(const char *path, int data);
int get_psu_info_wobmc(int id, int *mvin, int *mvout, int *mpin, int *mpout, int *miin, int *miout);
int get_psu_model_sn_wobmc(int id, char *model, char *serial_number);
int get_fan_info_wobmc(int id, char *model, char *serial, int *isfanb2f);
int get_fan_airflow(void);
int get_sensor_info_wobmc(int id, int *temp, int *warn, int *error, int *shutdown);
int get_fan_speed_wobmc(int id,int *per, int *rpm);


//CPLD
#define FAN_CPLD_BUS 7
#define FAN_CPLD_ADDR 0x0d
#define ABSENT 1
#define PRESENT 0


#endif /* _PLATFORM_DS2000_WOBMC_H_ */
