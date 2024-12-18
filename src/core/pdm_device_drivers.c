#include <linux/i2c.h>
#include <linux/i3c/master.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>

#include "pdm.h"
#include "pdm_device.h"
#include "pdm_driver_manager.h"
#include "pdm_device_drivers.h"

/**
 * @brief PDM 主模板驱动程序列表
 *
 * 该列表用于存储所有注册的 PDM 主模板驱动程序。
 */
static struct list_head pdm_device_driver_list;

/**
 * @brief PDM 主模板驱动程序数组
 *
 * 该数组包含所有需要注册的 PDM 主模板驱动程序。每个 `pdm_subdriver` 结构体包含驱动程序的名称、初始化函数和退出函数。
 */
static struct pdm_subdriver pdm_device_drivers[] = {
    {
        .name = "PDM SPI Device",
        .status = true,
        .ignore_failures = true,
        .init = pdm_device_spi_driver_init,
        .exit = pdm_device_spi_driver_exit
    },
    {
        .name = "PDM I2C Device",
        .status = true,
        .ignore_failures = true,
        .init = pdm_device_i2c_driver_init,
        .exit = pdm_device_i2c_driver_exit
    },
    {
        .name = "PDM PLATFORM Device",
        .status = true,
        .ignore_failures = true,
        .init = pdm_device_platform_driver_init,
        .exit = pdm_device_platform_driver_exit
    },
};

/**
 * @brief 初始化 PDM 设备
 *
 * 该函数用于初始化 PDM 设备，包括注册设备类和设备驱动。
 * 它会执行以下操作：
 * - 初始化子驱动列表
 * - 注册 PDM 设备驱动
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_device_drivers_register(void)
{
    struct pdm_subdriver_register_params params;
    int status;

    INIT_LIST_HEAD(&pdm_device_driver_list);

    params.drivers = pdm_device_drivers;
    params.count = ARRAY_SIZE(pdm_device_drivers);
    params.list = &pdm_device_driver_list;
    status = pdm_subdriver_register(&params);
    if (status < 0) {
        OSA_ERROR("Failed to register PDM Device Drivers, error: %d.\n", status);
        return status;
    }

    OSA_DEBUG("Initialize PDM Device Drivers OK.\n");
    return 0;
}

/**
 * @brief 卸载 PDM 设备
 *
 * 该函数用于卸载 PDM 设备，包括注销设备驱动。
 * 它会执行以下操作：
 * - 注销 PDM 设备驱动
 *
 * @note 在调用此函数之前，请确保所有相关的设备已经注销。
 */
void pdm_device_drivers_unregister(void)
{
    pdm_subdriver_unregister(&pdm_device_driver_list);
    OSA_DEBUG("PDM Device Drivers Exited.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Device Drivers");
