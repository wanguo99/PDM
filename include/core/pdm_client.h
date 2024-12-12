#ifndef _PDM_CLIENT_H_
#define _PDM_CLIENT_H_

/**
 * @brief PDM Client device name.
 */
#define PDM_CLIENT_DEVICE_NAME              "pdm_client"

/**
 * @brief Minimum minor number for PDM Client devices.
 */
#define PDM_CLIENT_MIN_MINOR                (0)

/**
 * @brief Maximum minor number for PDM Client devices.
 */
#define PDM_CLIENT_MAX_MINOR                (1023)

/**
 * @brief PDM Client structure.
 *
 * This structure defines the basic information for a PDM Client device.
 */
struct pdm_client {
    struct pdm_device *pdmdev;                  /**< Pointer to associated PDM device handle */
    struct pdm_adapter *adapter;                /**< Pointer to the owning PDM adapter */
    bool force_dts_id;                          /**< Flag indicating whether to force ID from DTS */
    int index;                                  /**< Client ID allocated by the adapter */
    struct device dev;                          /**< Kernel device structure */
    struct cdev cdev;                           /**< Character device structure for device operations */
    struct file_operations fops;                /**< File operations structure, defining operations for this device */
    struct list_head entry;                     /**< List node for linking devices */
};

/**
 * @brief Registers a PDM Client device.
 *
 * This function registers a new PDM Client device with the system.
 *
 * @param adapter Pointer to the PDM adapter.
 * @param client Pointer to the PDM Client device structure.
 * @return 0 on success, negative error code on failure.
 */
int pdm_client_register(struct pdm_adapter *adapter, struct pdm_client *client);

/**
 * @brief Unregisters a PDM Client device.
 *
 * This function unregisters a PDM Client device from the system and cleans up resources.
 *
 * @param adapter Pointer to the PDM adapter.
 * @param client Pointer to the PDM Client device structure.
 */
void pdm_client_unregister(struct pdm_adapter *adapter, struct pdm_client *client);

/**
 * @brief Allocates a new PDM Client device structure.
 *
 * This function allocates and initializes a new PDM Client device structure.
 *
 * @param data_size Size in bytes of additional data space to allocate for the structure.
 * @return Pointer to the allocated PDM Client device structure, or NULL on failure.
 */
struct pdm_client *pdm_client_alloc(unsigned int data_size);

/**
 * @brief Frees a PDM Client device structure.
 *
 * This function frees an allocated PDM Client device structure and its associated resources.
 *
 * @param client Pointer to the PDM Client device structure.
 */
void pdm_client_free(struct pdm_client *client);

/**
 * @brief Initializes the PDM Client module.
 *
 * This function initializes the PDM Client module, including registering necessary drivers and setting initial states.
 *
 * @return 0 on success, negative error code on failure.
 */
int pdm_client_init(void);

/**
 * @brief Exits the PDM Client module.
 *
 * This function exits the PDM Client module, including unregistering drivers and cleaning up all resources.
 */
void pdm_client_exit(void);

#endif /* _PDM_CLIENT_H_ */