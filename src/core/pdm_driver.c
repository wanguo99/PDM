#include "pdm.h"
#include "pdm_driver.h"
#include "pdm_template.h"


/**
 * @brief 注册一个子驱动
 *
 * 该函数用于注册单个 PDM 子驱动，调用其初始化函数并将其添加到子驱动链表中。
 *
 * @param driver 要注册的子驱动结构体指针
 * @param list 子驱动链表头指针
 * @return 成功返回 0，失败返回负错误码
 */
static int pdm_subdriver_register_single(struct pdm_subdriver *driver, struct list_head *list) {
    int ret = 0;
    if (driver->init) {
        ret = driver->init();
        if (ret) {
            OSA_ERROR("Failed to register driver %s, ret = %d.\n", driver->name ? driver->name : "Unknown", ret);
            return ret;
        }
    }
    list_add_tail(&driver->list, list);
    return 0;
}

/**
 * @brief 卸载一个子驱动
 *
 * 该函数用于卸载单个 PDM 子驱动，调用其退出函数并将其从子驱动链表中移除。
 *
 * @param driver 要卸载的子驱动结构体指针
 */
static void pdm_subdriver_unregister_single(struct pdm_subdriver *driver) {
    if (driver->exit) {
        driver->exit();
    }
    list_del(&driver->list);
}

/**
 * @brief 卸载链表中所有的驱动
 *
 * 该函数用于卸载所有已注册的 PDM 子驱动，依次调用每个子驱动的退出函数。
 *
 * @param list 子驱动链表头指针
 */
void pdm_subdriver_unregister(struct list_head *list)
{
    struct pdm_subdriver *driver, *tmp;

    if (!is_list_valid(list))
        return;

    list_for_each_entry_safe_reverse(driver, tmp, list, list) {
        pdm_subdriver_unregister_single(driver);
    }
}

/**
 * @brief 注册数组中所有的驱动并保存至链表
 *
 * 该函数用于注册所有 PDM 子驱动，依次调用每个子驱动的初始化函数。
 *
 * @param drivers 要注册的子驱动数组
 * @param count 子驱动数组的长度
 * @param list 子驱动链表头指针
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_subdriver_register(struct pdm_subdriver *drivers, int count, struct list_head *list)
{
    int i, ret = 0;

    if (!is_list_valid(list))
        return -EINVAL;

    for (i = 0; i < count; i++) {
        ret = pdm_subdriver_register_single(&drivers[i], list);
        if (ret) {
            OSA_ERROR("Failed to register driver %s at index %d\n", drivers[i].name, i);
            pdm_subdriver_unregister(list);
            return ret;
        }
    }
    OSA_INFO("PDM Sbudriver Register OK.\n");
    return 0;
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM SubDriver Module.");
