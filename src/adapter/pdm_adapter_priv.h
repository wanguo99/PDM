#ifndef _PDM_ADAPTER_PRIV_H_
#define _PDM_ADAPTER_PRIV_H_

/**
 * @brief 初始化 LED 主设备驱动
 *
 * 该函数用于初始化 LED 主设备驱动程序。
 *
 * @return 0 - 成功
 *         负值 - 失败
 */
int pdm_led_init(void);

/**
 * @brief 退出 LED 主设备驱动
 *
 * 该函数用于退出 LED 主设备驱动程序，并释放相关资源。
 */
void pdm_led_exit(void);


#endif /* _PDM_ADAPTER_PRIV_H_ */
