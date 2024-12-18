#include <linux/i2c.h>

#include "pdm.h"

/**
 * @brief 兼容旧内核版本的 i2c_device_id 结构体定义
 *
 * 该结构体用于兼容 Linux 内核版本低于 2.6.25 的情况。
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
struct i2c_device_id {
    char name[I2C_NAME_SIZE];
    kernel_ulong_t driver_data;
};
#endif

/**
 * @brief 实际的 I2C 探测函数
 *
 * 该函数用于处理 I2C 设备的探测操作，分配 PDM 设备并注册到主设备。
 *
 * @param client I2C 客户端指针
 * @param id I2C 设备 ID
 * @return 成功返回 0，失败返回负错误码
 */
static int pdm_device_i2c_real_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct pdm_device *pdmdev;
    int status;

    pdmdev = pdm_device_alloc();
    if (!pdmdev) {
        OSA_ERROR("Failed to allocate pdm_device.\n");
        return -ENOMEM;
    }

    pdmdev->physical_info.type = PDM_DEVICE_INTERFACE_TYPE_I2C;
    pdmdev->physical_info.device.i2cdev = client;
    pdmdev->physical_info.of_node = client->dev.of_node;
    status = pdm_device_register(pdmdev);
    if (status) {
        OSA_ERROR("Failed to register pdm device, status=%d.\n", status);
        goto free_pdmdev;
    }

    OSA_DEBUG("PDM I2C Device Probed.\n");
    return 0;

free_pdmdev:
    pdm_device_free(pdmdev);
    return status;
}

/**
 * @brief 实际的 I2C 移除函数
 *
 * 该函数用于处理 I2C 设备的移除操作，注销并释放 PDM 设备。
 *
 * @param client I2C 客户端指针
 * @return 成功返回 0，失败返回负错误码
 */
static int pdm_device_i2c_real_remove(struct i2c_client *client) {
    struct pdm_device *pdmdev;

    pdmdev = pdm_bus_find_device_by_of_node(client->dev.of_node);
    if (!pdmdev) {
        OSA_ERROR("Failed to find pdm device from bus.\n");
        return -ENODEV;
    } else {
        OSA_DEBUG("Found SPI PDM Device: %s", dev_name(&pdmdev->dev));
    }

    pdm_device_unregister(pdmdev);

    OSA_DEBUG("PDM I2C Device Removed.\n");
    return 0;
}

/**
 * @brief 兼容旧内核版本的 I2C 探测函数
 *
 * 该函数用于兼容 Linux 内核版本低于 6.3.0 的情况。
 *
 * @param client I2C 客户端指针
 * @param id I2C 设备 ID
 * @return 成功返回 0，失败返回负错误码
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
static int pdm_device_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    return pdm_device_i2c_real_probe(client, id);
}
#else
static int pdm_device_i2c_probe(struct i2c_client *client) {
    return pdm_device_i2c_real_probe(client, NULL);
}
#endif

/**
 * @brief 兼容旧内核版本的 I2C 移除函数
 *
 * 该函数用于兼容 Linux 内核版本低于 6.0.0 的情况。
 *
 * @param client I2C 客户端指针
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
static int pdm_device_i2c_remove(struct i2c_client *client) {
    return pdm_device_i2c_real_remove(client);
}
#else
static void pdm_device_i2c_remove(struct i2c_client *client) {
    (void)pdm_device_i2c_real_remove(client);
}
#endif

/**
 * @brief I2C 设备 ID 表
 *
 * 该表定义了支持的 I2C 设备 ID。
 */
static const struct i2c_device_id pdm_device_i2c_id[] = {
    { "pdm-device-i2c", 0 },
    { }  // 终止符
};
MODULE_DEVICE_TABLE(i2c, pdm_device_i2c_id);

/**
 * @brief I2C 驱动结构体
 *
 * 该结构体定义了 I2C 驱动的基本信息和操作函数。
 */
static struct i2c_driver pdm_device_i2c_driver = {
    .driver = {
        .name = "pdm-device-i2c",
        .owner = THIS_MODULE,
    },
    .probe = pdm_device_i2c_probe,
    .remove = pdm_device_i2c_remove,
    .id_table = pdm_device_i2c_id,
};

/**
 * @brief 初始化 I2C 驱动
 *
 * 该函数用于初始化 I2C 驱动，并将其注册到系统中。
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_device_i2c_driver_init(void) {
    int status;

    status = i2c_add_driver(&pdm_device_i2c_driver);
    if (status) {
        OSA_ERROR("Failed to register PDM Device I2C Driver, status=%d.\n", status);
        return status;
    }
    OSA_DEBUG("PDM Device I2C Driver Initialized.\n");
    return 0;
}

/**
 * @brief 退出 I2C 驱动
 *
 * 该函数用于退出 I2C 驱动，并将其从系统中注销。
 */
void pdm_device_i2c_driver_exit(void) {
    i2c_del_driver(&pdm_device_i2c_driver);
    OSA_DEBUG("PDM Device I2C Driver Exited.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Device I2C Driver.");
