#include <linux/spi/spi.h>

#include "pdm.h"
#include "pdm_device_priv.h"

/**
 * @brief Probes the SPI device and initializes the PDM device.
 *
 * This function is called when an SPI device is detected, responsible for initializing and registering the PDM device.
 *
 * @param spi Pointer to the SPI device structure.
 * @return Returns 0 on success; negative error code on failure.
 */
static int pdm_device_spi_probe(struct spi_device *spi) {
    struct pdm_device *pdmdev;
    int status;

    pdmdev = pdm_device_alloc(&spi->dev, sizeof(struct pdm_device_priv));
    if (IS_ERR(pdmdev)) {
        OSA_ERROR("Failed to allocate pdm_device\n");
        return PTR_ERR(pdmdev);
    }

    status = pdm_device_register(pdmdev);
    if (status) {
        OSA_ERROR("Failed to register pdm device, status=%d\n", status);
        goto err_pdmdev_free;
    }

    status = pdm_device_setup(pdmdev);
    if (status) {
        OSA_ERROR("Failed to setup pdm device, status=%d\n", status);
        goto err_pdmdev_unregister;
    }
    return 0;

err_pdmdev_unregister:
    pdm_device_unregister(pdmdev);
err_pdmdev_free:
    pdm_device_free(pdmdev);
    return status;
}

/**
 * @brief Removes the SPI device and unregisters the PDM device.
 *
 * This function is called when an SPI device is removed, responsible for unregistering and freeing the PDM device.
 *
 * @param spi Pointer to the SPI device structure.
 * @return Returns 0 on success; negative error code on failure.
 */
static int pdm_device_spi_real_remove(struct spi_device *spi) {
    struct pdm_device *pdmdev = pdm_bus_find_device_by_parent(&spi->dev);
    if (pdmdev) {
        pdm_device_cleanup(pdmdev);
        pdm_device_unregister(pdmdev);
        pdm_device_free(pdmdev);
    }
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
/**
 * @brief Compatibility remove function for older kernel versions.
 *
 * This function is used for Linux kernel versions below 5.17.0.
 *
 * @param spi Pointer to the SPI device structure.
 * @return Returns 0 on success; negative error code on failure.
 */
static int pdm_device_spi_remove(struct spi_device *spi) {
    return pdm_device_spi_real_remove(spi);
}
#else
/**
 * @brief Removes the SPI device and unregisters the PDM device.
 *
 * This function is called when an SPI device is removed, responsible for unregistering and freeing the PDM device.
 *
 * @param spi Pointer to the SPI device structure.
 */
static void pdm_device_spi_remove(struct spi_device *spi) {
    if (pdm_device_spi_real_remove(spi)) {
        OSA_ERROR("pdm_device_spi_real_remove failed\n");
    }
}
#endif

/**
 * @brief SPI device ID table.
 *
 * Defines the supported SPI device IDs.
 */
static const struct spi_device_id pdm_device_spi_ids[] = {
    { .name = "pdm,device-spi" },
    { }
};
MODULE_DEVICE_TABLE(spi, pdm_device_spi_ids);


/**
 * @brief Initializes GPIO settings for a PDM device.
 *
 * This function initializes the GPIO settings for the specified PDM device and sets up the operation functions.
 *
 * @param client Pointer to the PDM client structure.
 * @return Returns 0 on success; negative error code on failure.
 */
static int pdm_device_spi_setup(struct pdm_device *pdmdev)
{
    return 0;
}

static void pdm_device_spi_cleanup(struct pdm_device *pdmdev)
{
    return;
}

/**
 * @brief Match data structure for initializing GPIO type devices.
 */
static const struct pdm_device_match_data pdm_device_spi_match_data = {
    .setup = pdm_device_spi_setup,
    .cleanup = pdm_device_spi_cleanup,
};

/**
 * @brief DEVICE_TREE match table.
 *
 * Defines the supported DEVICE_TREE compatibility strings.
 */
static const struct of_device_id pdm_device_spi_of_match[] = {
    { .compatible = "pdm,device-spi",   .data = &pdm_device_spi_match_data },
    { }
};
MODULE_DEVICE_TABLE(of, pdm_device_spi_of_match);

/**
 * @brief SPI driver structure.
 *
 * Defines the basic information and operation functions of the SPI driver.
 */
static struct spi_driver pdm_device_spi_driver = {
    .probe = pdm_device_spi_probe,
    .remove = pdm_device_spi_remove,
    .id_table = pdm_device_spi_ids,
    .driver = {
        .name = "pdm-device-spi",
        .of_match_table = pdm_device_spi_of_match,
    },
};

/**
 * @brief Initializes the PDM device SPI driver.
 *
 * Registers the PDM device SPI driver with the system.
 *
 * @return Returns 0 on success; negative error code on failure.
 */
int pdm_device_spi_driver_init(void) {
    return spi_register_driver(&pdm_device_spi_driver);
}

/**
 * @brief Exits the PDM device SPI driver.
 *
 * Unregisters the PDM device SPI driver from the system.
 */
void pdm_device_spi_driver_exit(void) {
    spi_unregister_driver(&pdm_device_spi_driver);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Device SPI Driver");
