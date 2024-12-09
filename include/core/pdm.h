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

#include "osa_log.h"
#include "pdm_device.h"
#include "pdm_master.h"

/**
 * @file pdm.h
 * @brief PDM 模块公共头文件
 *
 * 本文件定义了 PDM 模块的公共数据类型、结构体和函数声明。
 */

/**
 * @brief PDM 总线上的设备ID范围
 */
#define PDM_BUS_DEVICE_IDR_START (0)           /**< 设备ID起始值 */
#define PDM_BUS_DEVICE_IDR_END   (1024)        /**< 设备ID结束值 */

/**
 * @brief DebugFS 和 ProcFS 目录名称
 */
#define PDM_DEBUG_FS_DIR_NAME    "pdm"         /**< debugfs 和 procfs 目录名 */

/**
 * @brief PDM BUS 私有数据结构
 *
 * 该结构体保存 PDM BUS 的私有数据。
 */
struct pdm_bus_private_data {
    struct idr device_idr;                     /**< 用于给子设备分配ID的 IDR */
    struct mutex idr_mutex_lock;               /**< 保护 IDR 的互斥锁 */
};

/**
 * @brief PDM 设备 ID 结构体
 *
 * 该结构体用于存储 PDM 设备的 ID 信息。
 */
struct pdm_device_id {
    char compatible[PDM_DEVICE_NAME_SIZE];     /**< 驱动匹配字符串 */
    kernel_ulong_t driver_data;                /**< 驱动私有数据 */
};

/**
 * @brief PDM 驱动结构体
 *
 * 该结构体定义了 PDM 驱动的基本信息。
 */
struct pdm_driver {
    struct device_driver driver;               /**< 设备驱动结构体 */
    const struct pdm_device_id *id_table;      /**< 设备ID表 */
    int (*probe)(struct pdm_device *dev);      /**< 探测函数 */
    void (*remove)(struct pdm_device *dev);    /**< 移除函数 */
};


/**
 * @struct pdm_subdriver
 * @brief PDM 子驱动结构体定义
 *
 * 该结构体用于定义 PDM 子驱动的基本信息和操作函数。
 */
struct pdm_subdriver {
    bool status;                /**< 驱动是否加载，默认为false，设置为true后开启 */
    bool ignore_failures;       /**< 是否忽略驱动初始化失败 */
    const char *name;           /**< 子驱动的名称 */
    int (*init)(void);          /**< 子驱动的初始化函数 */
    void (*exit)(void);         /**< 子驱动的退出函数 */
    struct list_head list;      /**< 用于链表管理的节点 */
};

/**
 * @brief 子驱动注册参数结构体
 *
 * 该结构体用于封装子驱动注册所需的所有参数。
 */
struct pdm_subdriver_register_params {
    struct pdm_subdriver *drivers;      /**< 要注册的子驱动数组 */
    int count;                          /**< 子驱动数组的长度 */
    struct list_head *list;             /**< 子驱动链表头指针 */
};

/**
 * @brief 卸载链表中所有的驱动
 *
 * 该函数用于卸载所有已注册的 PDM 子驱动，依次调用每个子驱动的退出函数。
 *
 * @param list 子驱动链表头指针
 */
void pdm_subdriver_unregister(struct list_head *list);

/**
 * @brief 注册数组中所有的驱动并保存至链表
 *
 * 该函数用于注册所有 PDM 子驱动，依次调用每个子驱动的初始化函数。
 *
 * @param params 子驱动注册参数结构体指针
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_subdriver_register(struct pdm_subdriver_register_params *params);


/**
 * @brief 为PDM设备分配ID
 *
 * 该函数为指定的 PDM 设备分配一个唯一的 ID。
 *
 * @param pdmdev PDM 设备指针
 * @return 成功返回 0，参数无效返回 -EINVAL，没有可用的ID返回 -EBUSY，其他错误码
 */
int pdm_bus_device_id_alloc(struct pdm_device *pdmdev);

/**
 * @brief 释放PDM设备的ID
 *
 * 该函数释放之前分配给 PDM 设备的 ID。
 *
 * @param pdmdev PDM 设备指针
 */
void pdm_bus_device_id_free(struct pdm_device *pdmdev);

/**
 * @brief 根据of_node值在pdm_bus_type 总线上查找设备
 *
 * 该函数遍历 `pdm_bus_type` 总线上的所有设备，找到of_node值匹配的设备。
 *
 * @param data 传递给回调函数的数据
 * @param fn 回调函数指针，用于处理每个设备
 * @return 返回遍历结果，0 表示成功，非零值表示失败
 */
struct pdm_device *pdm_bus_find_device_by_of_node(struct device_node *of_node);

/**
 * @brief 遍历 pdm_bus_type 总线上的所有设备
 *
 * 该函数遍历 `pdm_bus_type` 总线上的所有设备，并对每个设备调用指定的回调函数。
 *
 * @param data 传递给回调函数的数据
 * @param fn 回调函数指针，用于处理每个设备
 * @return 返回遍历结果，0 表示成功，非零值表示失败
 */
int pdm_bus_for_each_dev(void *data, int (*fn)(struct device *dev, void *data));

/**
 * @brief 注册 PDM 驱动
 *
 * 该函数注册 PDM 驱动到内核。
 *
 * @param owner 模块指针
 * @param driver PDM 驱动结构体指针
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_bus_register_driver(struct module *owner, struct pdm_driver *driver);

/**
 * @brief 注销 PDM 驱动
 *
 * 该函数从内核中注销 PDM 驱动。
 *
 * @param driver PDM 驱动结构体指针
 */
void pdm_bus_unregister_driver(struct pdm_driver *driver);

/**
 * @brief 将 device_driver 转换为 pdm_driver
 *
 * 该宏用于将 `device_driver` 结构体转换为 `pdm_driver` 结构体。
 *
 * @param __drv device_driver 结构体指针
 * @return pdm_driver 结构体指针
 */
#define drv_to_pdm_driver(__drv) container_of(__drv, struct pdm_driver, driver)

/**
 * @brief PDM 总线类型
 *
 * 该变量定义了 PDM 总线的基本信息和操作函数。
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
extern struct bus_type pdm_bus_type;
#else
extern const struct bus_type pdm_bus_type;
#endif

#endif /* _PDM_H_ */
