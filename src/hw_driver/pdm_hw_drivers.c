#include <linux/i2c.h>
#include <linux/i3c/master.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>

#include "pdm.h"
#include "pdm_component.h"
#include "pdm_hw_driver_priv.h"

/**
 * @brief PDM 主模板驱动程序列表
 *
 * 该列表用于存储所有注册的 PDM 主模板驱动程序。
 */
static struct list_head pdm_hw_drivers_list;

/**
 * @brief PDM 主模板驱动程序数组
 *
 * 该数组包含所有需要注册的 PDM 主模板驱动程序。每个 `pdm_component` 结构体包含驱动程序的名称、初始化函数和退出函数。
 */
static struct pdm_component pdm_hw_drivers[] = {
    {
        .name = "PDM Hardware Driver SPI",
        .status = true,
        .ignore_failures = true,
        .init = pdm_hw_driver_spi_init,
        .exit = pdm_hw_driver_spi_exit
    },
    {
        .name = "PDM Hardware Driver I2C",
        .status = true,
        .ignore_failures = true,
        .init = pdm_hw_driver_i2c_init,
        .exit = pdm_hw_driver_i2c_exit
    },
    {
        .name = "PDM Hardware Driver PLATFORM",
        .status = true,
        .ignore_failures = true,
        .init = pdm_hw_driver_platform_init,
        .exit = pdm_hw_driver_platform_exit
    },
};

/**
 * @brief 初始化 PDM hw_driver
 *
 * 该函数用于初始化 PDM hw_driver。
 * 它会执行以下操作：
 * - 初始化hw_driver列表
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_hw_drivers_register(void)
{
    struct pdm_component_params params;
    int status;

    INIT_LIST_HEAD(&pdm_hw_drivers_list);

    params.drivers = pdm_hw_drivers;
    params.count = ARRAY_SIZE(pdm_hw_drivers);
    params.list = &pdm_hw_drivers_list;
    status = pdm_component_register(&params);
    if (status < 0) {
        OSA_ERROR("Failed to register PDM Hardware Drivers, error: %d.\n", status);
        return status;
    }

    OSA_DEBUG("Initialize PDM Hardware Drivers OK.\n");
    return 0;
}

/**
 * @brief 卸载 PDM hw_driver
 *
 * 该函数用于卸载 PDM hw_driver。
 * 它会执行以下操作：
 * - 注销 PDM hw_driver
 *
 * @note 在调用此函数之前，请确保所有相关的设备已经注销。
 */
void pdm_hw_drivers_unregister(void)
{
    pdm_component_unregister(&pdm_hw_drivers_list);
    OSA_DEBUG("PDM Hardware Drivers Exited.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Hardware Drivers");
