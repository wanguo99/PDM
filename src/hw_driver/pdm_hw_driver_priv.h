#ifndef _PDM_HW_DRIVER_PRIV_H_
#define _PDM_HW_DRIVER_PRIV_H_

/**
 * @brief 初始化 I2C 驱动
 *
 * 该函数用于初始化 I2C 驱动并将其注册到系统中。
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_device_i2c_driver_init(void);

/**
 * @brief 退出 I2C 驱动
 *
 * 该函数用于退出 I2C 驱动并将其从系统中注销。
 */
void pdm_device_i2c_driver_exit(void);

/**
 * @brief 初始化 PLATFORM 驱动
 *
 * 该函数用于初始化 PLATFORM 驱动并将其注册到系统中。
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_device_platform_driver_init(void);

/**
 * @brief 退出 PLATFORM 驱动
 *
 * 该函数用于退出 PLATFORM 驱动并将其从系统中注销。
 */
void pdm_device_platform_driver_exit(void);

/**
 * @brief 初始化 SPI 驱动
 *
 * 该函数用于初始化 SPI 驱动并将其注册到系统中。
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_device_spi_driver_init(void);

/**
 * @brief 退出 SPI 驱动
 *
 * 该函数用于退出 SPI 驱动并将其从系统中注销。
 */
void pdm_device_spi_driver_exit(void);

#endif /* _PDM_HW_DRIVER_PRIV_H_ */
