#ifndef _PDM_DEVICE_H_
#define _PDM_DEVICE_H_

#include <linux/i2c.h>
#include <linux/i3c/master.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>

/**
 * @brief Maximum length of device names.
 */
#define PDM_DEVICE_NAME_SIZE (64)

/**
 * @struct pdm_device
 * @brief Structure defining a PDM device.
 *
 * Contains essential information about a PDM device, including its ID,
 * device structure, and client handle.
 */
struct pdm_device {
    unsigned int index;         /**< Device index. */
    struct device dev;          /**< Device structure. */
    struct pdm_client *client;  /**< PDM Client handle. */
    void *priv_data;            /**< PDM Device private data. */
};


/**
 * @struct pdm_device_gpio_data
 * @brief Data structure for GPIO-controlled PDM Devices.
 *
 * This structure holds the necessary data for controlling an device via GPIO.
 */
struct pdm_device_gpio {
    struct gpio_desc *gpiod;
};

/**
 * @struct pdm_device_pwm_data
 * @brief Data structure for PWM-controlled PDM Devices.
 *
 * This structure holds the necessary data for controlling an device via PWM.
 */
struct pdm_device_pwm {
    struct pwm_device *pwmdev;
};

/**
 * @struct pdm_device_spi_data
 * @brief Data structure for SPI-controlled PDM Devices.
 *
 * This structure holds the necessary data for controlling an device via SPI.
 */
struct pdm_device_spi {
    struct spi_device *spidev;
    struct regmap *map;
};

/**
 * @struct pdm_device_i2c_data
 * @brief Data structure for I2C-controlled PDM Devices.
 *
 * This structure holds the necessary data for controlling an device via I2C.
 */
struct pdm_device_i2c {
    struct i2c_client *client;
    struct regmap *map;
};

/**
* @union pdm_device_hw_data
 * @brief Union to hold hardware-specific data for different types of device controls.
 *
 * This union allows the same structure to accommodate different types of device control
 * mechanisms (e.g., GPIO or PWM).
 */
union pdm_hardware_device {
    struct pdm_device_gpio  gpio;
    struct pdm_device_pwm   pwm;
    struct pdm_device_spi   spi;
    struct pdm_device_i2c   i2c;
};


/**
 * @struct pdm_device_priv
 * @brief PDM Device Private Data Structure
 *
 * This structure is used to store private data for a PDM Device, including pointers to
 * operation functions.
 */
struct pdm_device_priv {
    union pdm_hardware_device hardware;
    const struct pdm_device_match_data *match_data;
};

/**
 * @def dev_to_pdm_device
 * @brief Converts a `device` pointer to a `pdm_device` pointer.
 *
 * This macro casts a `device` structure pointer to a `pdm_device` structure pointer.
 *
 * @param __dev Pointer to a `device` structure.
 * @return Pointer to a `pdm_device` structure.
 */
#define dev_to_pdm_device(__dev) container_of(__dev, struct pdm_device, dev)

/**
 * @brief Setup a PDM device.
 *
 * @param pdmdev Pointer to the PDM device structure.
 * @return 0 on success, negative error code on failure.
 */
int pdm_device_setup(struct pdm_device *pdmdev);

/**
 * @brief Cleanup a PDM device.
 *
 * @param pdmdev Pointer to the PDM device structure.
 */
void pdm_device_cleanup(struct pdm_device *pdmdev);

/**
 * @brief pdm_device_get.
 *
 * @param pdmdev Pointer to the PDM device structure.
 * @return Pointer to the PDM device structure, or NULL on failure.
 */
static inline struct pdm_device *pdm_device_get(struct pdm_device *pdmdev)
{
	return (pdmdev && get_device(&pdmdev->dev)) ? pdmdev : NULL;
}

/**
 * @brief pdm_device_put.
 *
 * @param pdmdev Pointer to the PDM device structure.
 */
static inline void pdm_device_put(struct pdm_device *pdmdev)
{
	if (pdmdev)
		put_device(&pdmdev->dev);
}

/**
 * @brief Retrieves the driver data associated with the device.
 *
 * This function retrieves the driver data stored in the device structure.
 *
 * @param Pointer to the PDM Device structure.
 * @return Pointer to the driver data stored in the device structure.
 */
static inline void *pdm_device_get_private_data(struct pdm_device *pdmdev)
{
    if (!pdmdev) {
        return NULL;
    }
    return pdmdev->priv_data;
}

/**
 * @brief Sets the driver data associated with the device.
 *
 * This function sets the driver data in the device structure.
 *
 * @param Pointer to the PDM Device structure.
 * @param data Pointer to the driver data to be set.
 */
static inline void pdm_device_set_private_data(struct pdm_device *pdmdev, void *data)
{
    pdmdev->priv_data = data;
}

/**
 * @brief Allocates a new PDM device structure.
 *
 * @return Pointer to the allocated PDM device structure, or NULL on failure.
 */
struct pdm_device *pdm_device_alloc(struct device *dev, unsigned int data_size);

/**
 * @brief Frees a PDM device structure.
 *
 * @param pdmdev Pointer to the PDM device structure.
 */
void pdm_device_free(struct pdm_device *pdmdev);

/**
 * @brief Registers a PDM device with the system.
 *
 * @param pdmdev Pointer to the PDM device structure.
 * @return 0 on success, negative error code on failure.
 */
int pdm_device_register(struct pdm_device *pdmdev);

/**
 * @brief Unregisters a PDM device from the system.
 *
 * @param pdmdev Pointer to the PDM device structure.
 */
void pdm_device_unregister(struct pdm_device *pdmdev);

/**
 * @brief Retrieves the device tree node for a PDM device's parent device.
 *
 * This function retrieves the device tree node associated with the parent device of the given PDM device.
 * It can be used to access properties or subnodes defined in the device tree for the parent device.
 *
 * @param pdmdev Pointer to the PDM device structure.
 * @return Pointer to the device_node structure if found; NULL otherwise.
 */
struct device_node *pdm_device_get_of_node(struct pdm_device *pdmdev);

/**
 * @brief Retrieves match data for a PDM device from the device tree.
 *
 * This function looks up the device tree to find matching data for the given PDM device,
 * which can be used for initialization or configuration.
 *
 * @param pdmdev Pointer to the PDM device structure.
 * @return Pointer to the match data if found; NULL otherwise.
 */
const void *pdm_device_get_match_data(struct pdm_device *pdmdev);

/**
 * @brief Initializes the PDM device module.
 *
 * @return 0 on success, negative error code on failure.
 */
int pdm_device_init(void);

/**
 * @brief Cleans up and unregisters the PDM device module.
 */
void pdm_device_exit(void);

#endif /* _PDM_DEVICE_H_ */
