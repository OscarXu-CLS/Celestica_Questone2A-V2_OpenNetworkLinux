#include <unistd.h>
#include <onlplib/mmap.h>
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include <fcntl.h>
#include "platform_common.h"


static onlp_thermal_info_t thermal_info[] = {
    { },
    { { ONLP_THERMAL_ID_CREATE(Base_Temp_U5_ID), "Base_Temp_U5", 0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | 
            ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0 
        },
    { { ONLP_THERMAL_ID_CREATE(Base_Temp_U56_ID), "Base_Temp_U56",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | 
            ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0 
        },
    { { ONLP_THERMAL_ID_CREATE(Switch_Temp_U31_ID), "Switch_Temp_U31",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | 
            ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD | ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD, 0 
        },
    { { ONLP_THERMAL_ID_CREATE(Switch_Temp_U30_ID), "Switch_Temp_U30",   0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0 
        },
    { { ONLP_THERMAL_ID_CREATE(Switch_Temp_U29_ID), "Switch_Temp_U29",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | 
            ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0 
        },
    { { ONLP_THERMAL_ID_CREATE(Switch_Temp_U28_ID), "Switch_Temp_U28",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | 
            ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0 
        }, 
    { { ONLP_THERMAL_ID_CREATE(CPU_Temp_ID), "CPU_Temp",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0 
        },
    { { ONLP_THERMAL_ID_CREATE(TEMP_DIMMA0_ID), "TEMP_DIMMA0",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0 
        },
    { { ONLP_THERMAL_ID_CREATE(TEMP_DIMMB0_ID), "TEMP_DIMMB0",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0
        },
    { { ONLP_THERMAL_ID_CREATE(VDD_CORE_Temp_ID), "VDD_CORE_Temp",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0, 
        },  
    { { ONLP_THERMAL_ID_CREATE(VDD_ANLG_Temp_ID), "VDD_ANLG_Temp",   0},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE, 0, 
        },
    { { ONLP_THERMAL_ID_CREATE(PSU1_Temp1_ID), "PSU1_Temp1",   ONLP_PSU_ID_CREATE(1)},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0, 
        },
    { { ONLP_THERMAL_ID_CREATE(PSU1_Temp2_ID), "PSU1_Temp2",   ONLP_PSU_ID_CREATE(1)},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0
        },
    { { ONLP_THERMAL_ID_CREATE(PSU2_Temp1_ID), "PSU2_Temp1",   ONLP_PSU_ID_CREATE(2)},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0
        },
    { { ONLP_THERMAL_ID_CREATE(PSU2_Temp2_ID), "PSU2_Temp2",   ONLP_PSU_ID_CREATE(2)},
            ONLP_THERMAL_STATUS_PRESENT,
            ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD, 0
        },
};

int onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t *info_p)
{
    int thermal_id;
    int thermal_status = 0;
    int temp, warn, err, shutdown;

    thermal_id = ONLP_OID_ID_GET(id);

    memcpy(info_p, &thermal_info[thermal_id], sizeof(onlp_thermal_info_t));

    /* Get thermal temperature. */
    thermal_status = get_sensor_info(thermal_id, &temp, &warn, &err, &shutdown);
    if (-1 == thermal_status)
    {
        info_p->status = ONLP_THERMAL_STATUS_FAILED;
    }
    else
    {
        info_p->status = ONLP_THERMAL_STATUS_PRESENT;
        info_p->mcelsius = temp;
        info_p->thresholds.warning = warn;
        info_p->thresholds.error = err;
        info_p->thresholds.shutdown = shutdown;
    }

    return ONLP_STATUS_OK;
}
