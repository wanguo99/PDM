#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/version.h>

#include "pdm.h"
#include "pdm_led.h"

static struct pdm_master *pdm_led_master;
static struct class *pdm_led_class;

int pdm_led_device_register(struct pdm_led_device *led_dev) {
    int ret;

    if (!pdm_led_master) {
        return -ENODEV;
    }

    led_dev->pdm_dev->master = pdm_led_master;

    // Register the device with the PDM core
    ret = pdm_device_register(led_dev->pdm_dev);
    if (ret) {
        return ret;
    }

    // Add the device to the master's list
    list_add_tail(&led_dev->pdm_dev->node, &pdm_led_master->slaves);

    return 0;
}

void pdm_led_device_unregister(struct pdm_led_device *led_dev) {
    if (led_dev && led_dev->pdm_dev) {
        // Remove the device from the master's list
        list_del(&led_dev->pdm_dev->node);

        // Unregister the device with the PDM core
        pdm_device_unregister(led_dev->pdm_dev);
        pdm_device_free(led_dev->pdm_dev);
    }
}

int pdm_led_device_alloc(struct pdm_led_device **led_dev) {
    int ret;
    struct pdm_device *base_dev;

    *led_dev = kzalloc(sizeof(**led_dev), GFP_KERNEL);
    if (!*led_dev) {
        return -ENOMEM;
    }

    ret = pdm_device_alloc(&base_dev, pdm_led_master);
    if (ret) {
        kfree(*led_dev);
        return ret;
    }

    (*led_dev)->pdm_dev = base_dev;

    return 0;
}

void pdm_led_device_free(struct pdm_led_device *led_dev) {
    if (led_dev) {
        if (led_dev->pdm_dev) {
            pdm_device_free(led_dev->pdm_dev);
        }
        kfree(led_dev);
    }
}

int pdm_led_master_init(void) {
    int ret;

    printk(KERN_INFO "LED Master initialized\n");

    ret = pdm_master_alloc(&pdm_led_master, "pdm_led");
    if (ret) {
        printk(KERN_ERR "[WANGUO] (%s:%d) Failed to allocate PDM master\n", __func__, __LINE__);
        return ret;
    }

    INIT_LIST_HEAD(&pdm_led_master->slaves);

    // Create the class for the PDM LED devices
#if LINUX_VERSION_CODE > KERNEL_VERSION(6, 7, 0)
    pdm_led_class = class_create("pdm_led_class");
#else
    pdm_led_class = class_create(THIS_MODULE, "pdm_led_class");
#endif
    if (IS_ERR(pdm_led_class)) {
        ret = PTR_ERR(pdm_led_class);
        printk(KERN_ERR "[WANGUO] (%s:%d) Failed to create class: %d\n", __func__, __LINE__, ret);
        pdm_master_free(pdm_led_master);
        return ret;
    }

    // Set the device name
    ret = dev_set_name(&pdm_led_master->dev, "pdm_led");
    if (ret) {
        printk(KERN_ERR "[WANGUO] (%s:%d) Failed to set device name: %d\n", __func__, __LINE__, ret);
        class_destroy(pdm_led_class);
        pdm_master_free(pdm_led_master);
        return ret;
    }

    pdm_led_master->dev.class = pdm_led_class;

    // Register the master device
    ret = pdm_master_register(pdm_led_master);
    if (ret) {
        printk(KERN_ERR "[WANGUO] (%s:%d) Failed to register PDM master: %d\n", __func__, __LINE__, ret);
        class_destroy(pdm_led_class);
        pdm_master_free(pdm_led_master);
        return ret;
    }

    return 0;
}

void pdm_led_master_exit(void) {
    printk(KERN_INFO "LED Master exited\n");

    // Unregister the master device
    pdm_master_unregister(pdm_led_master);

    // Free the master device
    pdm_master_free(pdm_led_master);

    // Destroy the class
    class_destroy(pdm_led_class);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wanguo");
MODULE_DESCRIPTION("PDM LED Master Module.");
