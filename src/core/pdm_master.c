#include <linux/slab.h>
#include <linux/platform_device.h>

#include "pdm.h"

static LIST_HEAD(pdm_master_list);
static DEFINE_MUTEX(pdm_master_list_mutex_lock);


/*                                                                              */
/*                            pdm_master_type                                   */
/*                                                                              */

/*
    pdm_master->file_operations
*/

static int pdm_master_fops_open_default(struct inode *inode, struct file *filp)
{
    OSA_INFO("Open function called.\n");
    return 0;
}

static int pdm_master_fops_release_default(struct inode *inode, struct file *filp)
{
    OSA_INFO("Release function called.\n");
    return 0;
}

static ssize_t pdm_master_fops_read_default(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    OSA_INFO("Read function called.\n");
    return 0;
}

static ssize_t pdm_master_fops_write_default(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    OSA_INFO("Write function called.\n");
    return 0;
}

static long pdm_master_fops_unlocked_ioctl_default(struct file *filp, unsigned int cmd, unsigned long arg)
{
    OSA_INFO("Master does not support ioctl operations.\n");
    return -ENOTSUPP;
}


/*
    pdm_master_device_type
*/
static ssize_t name_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct pdm_master *master = dev_to_pdm_master(dev);
    ssize_t ret;

    down_read(&master->rwlock);
    ret = sysfs_emit(buf, "%s\n", master->name);
    up_read(&master->rwlock);

    return ret;
}

static DEVICE_ATTR_RO(name);

static struct attribute *pdm_master_device_attrs[] = {
    &dev_attr_name.attr,
    NULL,
};
ATTRIBUTE_GROUPS(pdm_master_device);

static const struct device_type pdm_master_device_type = {
    .groups = pdm_master_device_groups,
};

static void pdm_master_dev_release(struct device *dev)
{
    struct pdm_master *master = dev_to_pdm_master(dev);

    OSA_INFO("Master %s released.\n", dev_name(&master->dev));
    kfree(master);
}

struct class pdm_master_class = {
    .name = "pdm_master",
    .dev_release = pdm_master_dev_release,
};

static int pdm_master_add_cdev(struct pdm_master *master)
{
    int ret;

    ret = alloc_chrdev_region(&master->devno, 0, 1, dev_name(&master->dev));
    if (ret < 0) {
        OSA_ERROR("Failed to allocate char device region for %s, error: %d.\n", dev_name(&master->dev), ret);
        return ret;
    }

    master->fops.open = pdm_master_fops_open_default;
    master->fops.release = pdm_master_fops_release_default;
    master->fops.read = pdm_master_fops_read_default;
    master->fops.write = pdm_master_fops_write_default;
    master->fops.unlocked_ioctl = pdm_master_fops_unlocked_ioctl_default;

    cdev_init(&master->cdev, &master->fops);
    master->cdev.owner = THIS_MODULE;
    ret = cdev_add(&master->cdev, master->devno, 1);
    if (ret < 0) {
        unregister_chrdev_region(master->devno, 1);
        OSA_ERROR("Failed to add char device for %s, error: %d.\n", dev_name(&master->dev), ret);
        return ret;
    }

    if (!device_create(&pdm_master_class, NULL, master->devno, NULL, "pdm_master_%s_cdev", master->name)) {
        cdev_del(&master->cdev);
        unregister_chrdev_region(master->devno, 1);
        OSA_ERROR("Failed to create device for %s.\n", master->name);
        return -ENOMEM;
    }

    OSA_INFO("Add cdev for %s ok.\n", dev_name(&master->dev));

    return 0;
}

static void pdm_master_delete_cdev(struct pdm_master *master)
{
    device_destroy(&pdm_master_class, master->devno);
    cdev_del(&master->cdev);
    unregister_chrdev_region(master->devno, 1);
}

void *pdm_master_get_devdata(struct pdm_master *master)
{
    return dev_get_drvdata(&master->dev);
}

void pdm_master_set_devdata(struct pdm_master *master, void *data)
{
    dev_set_drvdata(&master->dev, data);
}

struct pdm_master *pdm_master_get(struct pdm_master *master)
{
    if (!master || !get_device(&master->dev))
        return NULL;
    return master;
}

void pdm_master_put(struct pdm_master *master)
{
    if (master) {
        put_device(&master->dev);
    }
}

struct pdm_master *pdm_master_alloc(unsigned int data_size)
{
    struct pdm_master *master;
    size_t master_size = sizeof(struct pdm_master);

    master = kzalloc(master_size + data_size, GFP_KERNEL);
    if (!master) {
        OSA_ERROR("Failed to allocate memory for pdm_master");
        return NULL;
    }

    device_initialize(&master->dev);
    master->dev.release = pdm_master_dev_release;
    pdm_master_set_devdata(master, (void *)master + master_size);

    return master;
}

void pdm_master_free(struct pdm_master *master)
{
    if (master) {
        pdm_master_put(master);
    }
}

int pdm_master_id_alloc(struct pdm_master *master, struct pdm_device *pdmdev)
{
	int id;

    if (!master || !pdmdev) {
        pr_err("Invalid input parameters\n");
        return -EINVAL;
    }

	mutex_lock(&master->idr_mutex_lock);
    id = idr_alloc(&master->device_idr, pdmdev, PDM_MASTER_IDR_START, PDM_MASTER_IDR_END, GFP_KERNEL);
	mutex_unlock(&master->idr_mutex_lock);
    if (id < 0) {
        if (id == -ENOSPC) {
            pr_err("No available IDs in the range\n");
            return -EBUSY;
        } else {
            pr_err("Failed to allocate ID: %d\n", id);
            return id;
        }
    }

	pdmdev->id = id;
	return 0;
}

void pdm_master_id_free(struct pdm_master *master, struct pdm_device *pdmdev)
{
    if (!master || !pdmdev) {
        pr_err("Invalid input parameters\n");
        return;
    }

    mutex_lock(&master->idr_mutex_lock);
    idr_remove(&master->device_idr, pdmdev->id);
    mutex_unlock(&master->idr_mutex_lock);
}

int pdm_master_register(struct pdm_master *master)
{
    int ret;
    struct pdm_master *existing_master;

    if (!strlen(master->name)) {
        OSA_ERROR("Master name not set.\n");
        return -EINVAL;
    }

    if (!pdm_master_get(master))
        return -EBUSY;

    mutex_lock(&pdm_master_list_mutex_lock);
    list_for_each_entry(existing_master, &pdm_master_list, entry) {
        if (!strcmp(existing_master->name, master->name)) {
            OSA_ERROR("Master already exists: %s.\n", master->name);
            mutex_unlock(&pdm_master_list_mutex_lock);
            ret = -EEXIST;
            goto err_device_put;
        }
    }
    mutex_unlock(&pdm_master_list_mutex_lock);

    master->dev.type = &pdm_master_device_type;
    master->dev.class = &pdm_master_class;
    dev_set_name(&master->dev, "pdm_master_%s", master->name);

    ret = device_add(&master->dev);
    if (ret) {
        OSA_ERROR("Failed to add device: %s, error: %d.\n", dev_name(&master->dev), ret);
        goto err_device_put;
    }

    ret = pdm_master_add_cdev(master);
    if (ret) {
        OSA_ERROR("Failed to add cdev, error: %d.\n", ret);
        goto err_device_unregister;
    }

    mutex_lock(&pdm_master_list_mutex_lock);
    list_add_tail(&master->entry, &pdm_master_list);
    mutex_unlock(&pdm_master_list_mutex_lock);

    mutex_lock(&master->idr_mutex_lock);
    idr_init(&master->device_idr);
    mutex_unlock(&master->idr_mutex_lock);

    master->init_done = true;
    OSA_INFO("PDM Master Registered: %s.\n", dev_name(&master->dev));

    return 0;

err_device_unregister:
    device_unregister(&master->dev);

err_device_put:
    pdm_master_put(master);
    return ret;
}

void pdm_master_unregister(struct pdm_master *master)
{
    if (NULL == master) {
        OSA_ERROR("pdm_master_unregister: master is NULL\n");
        return;
    }

    mutex_lock(&master->client_list_mutex_lock);
    WARN_ONCE(!list_empty(&master->client_list), "Not all clients removed.");
    mutex_unlock(&master->client_list_mutex_lock);

    master->init_done = false;

    mutex_lock(&pdm_master_list_mutex_lock);
    list_del(&master->entry);
    mutex_unlock(&pdm_master_list_mutex_lock);

    mutex_lock(&master->idr_mutex_lock);
    idr_destroy(&master->device_idr);
    mutex_unlock(&master->idr_mutex_lock);

    pdm_master_delete_cdev(master);
    device_unregister(&master->dev);
    OSA_INFO("PDM Master unregistered: %s.\n", dev_name(&master->dev));
}

int pdm_master_add_device(struct pdm_master *master, struct pdm_device *pdmdev)
{
    if (master)
    {
        printk(KERN_ERR "[WANGUO] (%s:%d) \n", __func__, __LINE__);
    }
    if (pdmdev)
    {
        printk(KERN_ERR "[WANGUO] (%s:%d) \n", __func__, __LINE__);
    }
    pdmdev->master = master;
    mutex_lock(&master->client_list_mutex_lock);
    list_add_tail(&pdmdev->entry, &master->client_list);
    mutex_unlock(&master->client_list_mutex_lock);

    return 0;
}

int pdm_master_delete_device(struct pdm_master *master, struct pdm_device *pdmdev)
{
    mutex_lock(&master->client_list_mutex_lock);
    list_del(&pdmdev->entry);
    mutex_unlock(&master->client_list_mutex_lock);

    return 0;
}

struct pdm_device *pdm_master_find_pdmdev(struct pdm_master *master, void *real_device)
{
    struct pdm_device *existing_pdmdev;

    mutex_lock(&master->client_list_mutex_lock);
    list_for_each_entry(existing_pdmdev, &master->client_list, entry) {
        if (existing_pdmdev->real_device == real_device) {
            mutex_unlock(&master->client_list_mutex_lock);
            return existing_pdmdev;
        }
    }
    mutex_unlock(&master->client_list_mutex_lock);

    OSA_ERROR("Failed to find device for real_device at %p.\n", real_device);
    return NULL;
}

int pdm_master_init(void)
{
    int ret = class_register(&pdm_master_class);
    if (ret < 0) {
        OSA_ERROR("PDM: Failed to register class\n");
        return ret;
    }
    OSA_INFO("Register PDM Master Class.\n");

    return 0;
}

void pdm_master_exit(void)
{
    class_unregister(&pdm_master_class);
    OSA_INFO("Unregister PDM Master Class.\n");
}
