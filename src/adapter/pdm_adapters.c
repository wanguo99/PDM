#include "pdm.h"
#include "pdm_component.h"
#include "pdm_adapter_priv.h"

/**
 * @brief PDM 主驱动程序列表
 *
 * 该列表用于存储所有注册的 PDM 主驱动程序。
 */
static struct list_head pdm_adapters_list;

/**
 * @brief PDM 主驱动程序数组
 *
 * 该数组包含所有需要注册的 PDM 主驱动程序。
 */
static struct pdm_component pdm_adapters[] = {
    {
        .name = "LED Adapter",
        .status = true,
        .ignore_failures = true,
        .init = pdm_led_init,
        .exit = pdm_led_exit,
    },
};

/**
 * @brief 初始化 PDM 主控制器模块
 *
 * 该函数用于初始化 PDM 主控制器模块，并注册所有 PDM 主驱动程序。
 *
 * @return 0 - 成功
 *         负值 - 失败
 */
int pdm_adapters_register(void)
{
    int status;
    struct pdm_component_params params;

    INIT_LIST_HEAD(&pdm_adapters_list);
    params.drivers = pdm_adapters;
    params.count = ARRAY_SIZE(pdm_adapters);
    params.list = &pdm_adapters_list;
    status = pdm_component_register(&params);
    if (status < 0) {
        OSA_ERROR("Failed to register PDM Adapters, error: %d.\n", status);
        return status;
    }

    OSA_DEBUG("PDM Adapters Registered OK.\n");
    return 0;
}

/**
 * @brief 卸载 PDM 主控制器模块
 *
 * 该函数用于卸载 PDM 主控制器模块，并注销所有 PDM 主驱动程序。
 */
void pdm_adapters_unregister(void)
{
    pdm_component_unregister(&pdm_adapters_list);

    OSA_DEBUG("PDM Adapters Unregistered.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Adapter Module");
