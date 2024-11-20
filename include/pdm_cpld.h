#ifndef _PDM_CPLD_H_
#define _PDM_CPLD_H_

#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include "pdm.h"

struct pdm_cpld_operations {
    int (*read_reg)(int addr, int *value);
    int (*write_reg)(int addr, int value);
};

struct pdm_cpld_master {
    struct pdm_master *master;
    struct pdm_cpld_operations ops;
};

struct pdm_cpld_device {
    struct pdm_device *pdmdev;
    union {
        struct i2c_client i2cdev;
        struct spi_device spidev;
    }client;
};

// Master initialization and exit functions
int pdm_cpld_master_init(void);
void pdm_cpld_master_exit(void);

// Driver initialization and exit functions
int pdm_cpld_i2c_driver_init(void);
void pdm_cpld_i2c_driver_exit(void);

#endif /* _PDM_CPLD_H_ */
