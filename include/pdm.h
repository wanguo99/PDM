#ifndef _PDM_H_
#define _PDM_H_

#include <linux/version.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>

#include "osa/osa_log.h"
#include "pdm_device.h"
#include "pdm_master.h"

/**
 * @file pdm.h
 * @brief PDM 模块公共头文件
 *
 * 本文件定义了 PDM 模块的公共数据类型、结构体和函数声明。
 */

/**
 * @brief PDM 总线上的设备的ID范围
 */
#define PDM_BUS_DEVICE_IDR_START        (0)
#define PDM_BUS_DEVICE_IDR_END          (1024)

/**
 * @brief DebugFS 和 ProcFS 目录名称
 */
#define PDM_DEBUG_FS_DIR_NAME           "pdm"       /**< debugfs和procfs目录名 */

/**
 * @brief PDM BUS结构体
 *
 * 保存PDM BUS私有数据。
 */
struct pdm_bus_private_data {
    struct idr device_idr;               /**< 用于给子设备分配ID的IDR */
    struct mutex idr_mutex_lock;         /**< 用于保护IDR的互斥锁 */
};

/**
 * @brief PDM 设备ID结构体
 */
struct pdm_device_id {
    char compatible[PDM_DEVICE_NAME_SIZE];      /**< 驱动匹配字符串 */
    kernel_ulong_t driver_data;                 /**< 驱动私有数据 */
};

/**
 * @brief PDM 驱动结构体
 */
struct pdm_driver {
    struct device_driver driver;                /**< 设备驱动结构体 */
    const struct pdm_device_id *id_table;       /**< 设备ID表 */
    int (*probe)(struct pdm_device *dev);       /**< 探测函数 */
    void (*remove)(struct pdm_device *dev);     /**< 移除函数 */
};

/**
 * @brief 为PDM设备分配ID
 * @master: PDM主控制器
 * @pdmdev: PDM设备
 *
 * 返回值:
 * 0 - 成功
 * -EINVAL - 参数无效
 * -EBUSY - 没有可用的ID
 * 其他负值 - 其他错误码
 */
int pdm_bus_device_id_alloc(struct pdm_device *pdmdev);


/**
 * @brief 释放PDM设备的ID
 * @master: PDM主控制器
 * @pdmdev: PDM设备
 */
void pdm_bus_device_id_free(struct pdm_device *pdmdev);

/**
 * @brief 遍历 pdm_bus_type 总线上的所有设备
 *
 * 该函数用于遍历 `pdm_bus_type` 总线上的所有设备，并对每个设备调用指定的回调函数。
 *
 * @param data 传递给回调函数的数据
 * @param fn 回调函数指针，用于处理每个设备
 * @return 返回遍历结果，0 表示成功，非零值表示失败
 */
int pdm_bus_for_each_dev(void *data, int (*fn)(struct device *dev, void *data));

/**
 * @brief 注册 PDM 驱动
 *
 * 该函数用于注册 PDM 驱动。
 *
 * @param owner 模块指针
 * @param driver 驱动结构体指针
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_bus_register_driver(struct module *owner, struct pdm_driver *driver);

/**
 * @brief 注销 PDM 驱动
 *
 * 该函数用于注销 PDM 驱动。
 *
 * @param driver 驱动结构体指针
 */
void pdm_bus_unregister_driver(struct pdm_driver *driver);

/**
 * @brief 将 driver 转换为 pdm_driver
 *
 * 该宏用于将 driver 转换为 pdm_driver
 *
 * @param __dev driver 结构体指针
 * @return pdm_driver 结构体指针
 */
#define drv_to_pdm_driver(__drv) container_of(__drv, struct pdm_driver, driver)


/**
 * @brief PDM 总线类型
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
extern struct bus_type pdm_bus_type;
#else
extern const struct bus_type pdm_bus_type;
#endif

#endif /* _PDM_H_ */
