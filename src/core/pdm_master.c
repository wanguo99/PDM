#include "pdm.h"
#include "pdm_subdriver.h"
#include "pdm_template.h"

static LIST_HEAD(pdm_master_list);
static DEFINE_MUTEX(pdm_master_list_mutex_lock);

static LIST_HEAD(pdm_master_driver_list);
static struct pdm_subdriver pdm_master_drivers[] = {
    { .name = "Template Master", .init = pdm_template_master_init, .exit = pdm_template_master_exit },
};

/**
 * pdm_master_client_id_alloc - 为PDM设备分配ID
 * @master: PDM主控制器
 * @pdmdev: PDM设备
 *
 * 返回值:
 * 0 - 成功
 * -EINVAL - 参数无效
 * -EBUSY - 没有可用的ID
 * 其他负值 - 其他错误码
 */
int pdm_master_client_id_alloc(struct pdm_master *master, struct pdm_device *pdmdev)
{
    int id;

    if (!master || !pdmdev) {
        OSA_ERROR("Invalid input parameters (master: %p, pdmdev: %p).\n", master, pdmdev);
        return -EINVAL;
    }

    mutex_lock(&master->idr_mutex_lock);
    id = idr_alloc(&master->device_idr, pdmdev, PDM_MASTER_CLIENT_IDR_START, PDM_MASTER_CLIENT_IDR_END, GFP_KERNEL);
    mutex_unlock(&master->idr_mutex_lock);

    if (id < 0) {
        if (id == -ENOSPC) {
            OSA_ERROR("No available IDs in the range.\n");
            return -EBUSY;
        } else {
            OSA_ERROR("Failed to allocate ID: %d.\n", id);
            return id;
        }
    }

    pdmdev->id = id;
    return 0;
}

/**
 * pdm_master_client_id_free - 释放PDM设备的ID
 * @master: PDM主控制器
 * @pdmdev: PDM设备
 */
void pdm_master_client_id_free(struct pdm_master *master, struct pdm_device *pdmdev)
{
    if (!master || !pdmdev) {
        OSA_ERROR("Invalid input parameters (master: %p, pdmdev: %p).\n", master, pdmdev);
        return;
    }

    mutex_lock(&master->idr_mutex_lock);
    idr_remove(&master->device_idr, pdmdev->id);
    mutex_unlock(&master->idr_mutex_lock);
}

/**
 * @brief 显示所有已注册的 PDM 设备列表
 *
 * 该函数用于显示当前已注册的所有 PDM 设备的名称。
 * 如果主设备未初始化，则会返回错误。
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_master_client_show(struct pdm_master *master)
{
    struct pdm_device *client;
    int index = 1;

    if (!master) {
        OSA_ERROR("Master is not initialized.\n");
        return -ENODEV;
    }

    OSA_INFO("-------------------------\n");
    OSA_INFO("Device List:\n");

    mutex_lock(&master->client_list_mutex_lock);
    list_for_each_entry(client, &master->client_list, entry) {
        OSA_INFO("  [%d] Client Name: %s.\n", index++, dev_name(&client->dev));
    }
    mutex_unlock(&master->client_list_mutex_lock);

    OSA_INFO("-------------------------\n");

    return 0;
}

/**
 * pdm_master_client_find - 查找与给定实际设备关联的PDM设备
 * @master: PDM主控制器
 * @real_device: 实际设备指针
 *
 * 返回值:
 * 指向找到的PDM设备的指针，或NULL（未找到）
 */
struct pdm_device *pdm_master_client_find(struct pdm_master *master, void *real_device)
{
    struct pdm_device *existing_pdmdev;

    if (!master) {
        OSA_ERROR("Invalid input parameter (master: %p).\n", master);
        return NULL;
    }

    mutex_lock(&master->client_list_mutex_lock);
    list_for_each_entry(existing_pdmdev, &master->client_list, entry) {
        if (existing_pdmdev->real_device == real_device) {
            mutex_unlock(&master->client_list_mutex_lock);
            OSA_DEBUG("Device found for real_device at %p.\n", real_device);
            return existing_pdmdev;
        }
    }
    mutex_unlock(&master->client_list_mutex_lock);
    OSA_ERROR("Failed to find device for real_device at %p.\n", real_device);
    return NULL;
}

/**
 * pdm_master_client_add - 向PDM主控制器添加设备
 * @master: PDM主控制器
 * @pdmdev: 要添加的PDM设备
 *
 * 返回值:
 * 0 - 成功
 * -EINVAL - 参数无效
 */
int pdm_master_client_add(struct pdm_master *master, struct pdm_device *pdmdev)
{
    if (!master || !pdmdev) {
        OSA_ERROR("Invalid input parameters (master: %p, pdmdev: %p).\n", master, pdmdev);
        return -EINVAL;
    }

    pdmdev->master = master;
    mutex_lock(&master->client_list_mutex_lock);
    list_add_tail(&pdmdev->entry, &master->client_list);
    mutex_unlock(&master->client_list_mutex_lock);
    return 0;
}

/**
 * pdm_master_client_delete - 从PDM主控制器删除设备
 * @master: PDM主控制器
 * @pdmdev: 要删除的PDM设备
 *
 * 返回值:
 * 0 - 成功
 * -EINVAL - 参数无效
 */
int pdm_master_client_delete(struct pdm_master *master, struct pdm_device *pdmdev)
{
    if (!master || !pdmdev) {
        OSA_ERROR("Invalid input parameters (master: %p, pdmdev: %p).\n", master, pdmdev);
        return -EINVAL;
    }

    mutex_lock(&master->client_list_mutex_lock);
    list_del(&pdmdev->entry);
    mutex_unlock(&master->client_list_mutex_lock);
    OSA_DEBUG("Device %s removed from %s master.\n", dev_name(&pdmdev->dev), master->name);
    return 0;
}



/**
 * pdm_master_class - PDM主控制器类
 */
static struct class pdm_master_class = {
    .name = "pdm_master",
};


/**
 * name_show - 显示设备名称
 * @dev: 设备结构
 * @da: 设备属性结构
 * @buf: 输出缓冲区
 *
 * 返回值:
 * 实际写入的字节数
 */
static ssize_t name_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct pdm_master *master = dev_to_pdm_master(dev);
    ssize_t ret;

    down_read(&master->rwlock);
    ret = sysfs_emit(buf, "%s\n", master->name);
    up_read(&master->rwlock);

    OSA_INFO("Device name: %s.\n", master->name);
    return ret;
}

static DEVICE_ATTR_RO(name);

static struct attribute *pdm_master_device_attrs[] = {
    &dev_attr_name.attr,
    NULL,
};
ATTRIBUTE_GROUPS(pdm_master_device);

/**
 * pdm_master_device_type - PDM主控制器设备类型
 */
static const struct device_type pdm_master_device_type = {
    .groups = pdm_master_device_groups,
};

/**
 * pdm_master_dev_release - 释放PDM主控制器设备
 * @dev: 设备结构
 */
static void pdm_master_dev_release(struct device *dev)
{
    struct pdm_master *master = dev_to_pdm_master(dev);
    kfree(master);
}


/**
 * pdm_master_fops_default_open - 默认打开函数
 * @inode: inode 结构
 * @filp: 文件结构
 *
 * 返回值:
 * 0 - 成功
 */
static int pdm_master_fops_default_open(struct inode *inode, struct file *filp)
{
    struct pdm_master *master;

    OSA_INFO("fops_default_open.\n");

    master = container_of(inode->i_cdev, struct pdm_master, cdev);
    if (!master)
    {
        OSA_ERROR("Invalid master.\n");
        return -EINVAL;
    }

    filp->private_data = master;

    return 0;
}

/**
 * pdm_master_fops_default_release - 默认释放函数
 * @inode: inode 结构
 * @filp: 文件结构
 *
 * 返回值:
 * 0 - 成功
 */
static int pdm_master_fops_default_release(struct inode *inode, struct file *filp)
{
    OSA_INFO("fops_default_release.\n");
    return 0;
}

/**
 * pdm_master_fops_default_read - 默认读取函数
 * @filp: 文件结构
 * @buf: 用户空间缓冲区
 * @count: 要读取的字节数
 * @ppos: 当前文件位置
 *
 * 返回值:
 * 0 - 成功
 */
static ssize_t pdm_master_fops_default_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    struct pdm_master *master;

    OSA_INFO("fops_default_read.\n");

    master = filp->private_data;
    if (!master)
    {
        OSA_ERROR("Invalid master.\n");
        return -EINVAL;
    }
    (void)pdm_master_client_show(master);
    return 0;
}

/**
 * pdm_master_fops_default_write - 默认写入函数
 * @filp: 文件结构
 * @buf: 用户空间缓冲区
 * @count: 要写入的字节数
 * @ppos: 当前文件位置
 *
 * 返回值:
 * 0 - 成功
 */
static ssize_t pdm_master_fops_default_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    OSA_INFO("fops_default_write.\n");
    return 0;
}

/**
 * pdm_master_fops_default_unlocked_ioctl - 默认ioctl函数
 * @filp: 文件结构
 * @cmd: ioctl命令
 * @arg: 命令参数
 *
 * 返回值:
 * -ENOTSUPP - 不支持的ioctl操作
 */
static long pdm_master_fops_default_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    OSA_INFO("This master does not support ioctl operations.\n");
    return -ENOTSUPP;
}


/**
 * pdm_master_cdev_add - 添加PDM主控制器字符设备
 * @master: PDM主控制器
 *
 * 返回值:
 * 0 - 成功
 * 负值 - 失败
 */
static int pdm_master_cdev_add(struct pdm_master *master)
{
    int ret;

    if (!master) {
        OSA_ERROR("Invalid input parameter (master: %p).\n", master);
        return -EINVAL;
    }

    ret = alloc_chrdev_region(&master->devno, 0, 1, dev_name(&master->dev));
    if (ret < 0) {
        OSA_ERROR("Failed to allocate char device region for %s, error: %d.\n", dev_name(&master->dev), ret);
        goto err_out;
    }

    master->fops.open = pdm_master_fops_default_open;
    master->fops.release = pdm_master_fops_default_release;
    master->fops.read = pdm_master_fops_default_read;
    master->fops.write = pdm_master_fops_default_write;
    master->fops.unlocked_ioctl = pdm_master_fops_default_ioctl;

    cdev_init(&master->cdev, &master->fops);
    master->cdev.owner = THIS_MODULE;
    ret = cdev_add(&master->cdev, master->devno, 1);
    if (ret < 0) {
        OSA_ERROR("Failed to add char device for %s, error: %d.\n", dev_name(&master->dev), ret);
        goto err_unregister_chrdev;
    }

    if (!device_create(&pdm_master_class, NULL, master->devno, NULL, "pdm_master_%s", master->name)) {
        OSA_ERROR("Failed to create device for %s.\n", master->name);
        goto err_cdev_del;
    }

    OSA_DEBUG("Add cdev for %s ok.\n", dev_name(&master->dev));

    return 0;

err_cdev_del:
    cdev_del(&master->cdev);
err_unregister_chrdev:
    unregister_chrdev_region(master->devno, 1);
err_out:
    return ret;
}


/**
 * pdm_master_cdev_delete - 删除PDM主控制器字符设备
 * @master: PDM主控制器
 */
static void pdm_master_cdev_delete(struct pdm_master *master)
{
    if (!master) {
        OSA_ERROR("Invalid input parameter (master: %p).\n", master);
        return;
    }

    device_destroy(&pdm_master_class, master->devno);
    cdev_del(&master->cdev);
    unregister_chrdev_region(master->devno, 1);
}


/**
 * pdm_master_devdata_get - 获取PDM主控制器的私有数据
 * @master: PDM主控制器
 *
 * 返回值:
 * 指向私有数据的指针
 */
void *pdm_master_devdata_get(struct pdm_master *master)
{
    if (!master) {
        OSA_ERROR("Invalid input parameter (master: %p).\n", master);
        return NULL;
    }

    return dev_get_drvdata(&master->dev);
}

/**
 * pdm_master_devdata_set - 设置PDM主控制器的私有数据
 * @master: PDM主控制器
 * @data: 私有数据指针
 */
void pdm_master_devdata_set(struct pdm_master *master, void *data)
{
    if (!master) {
        OSA_ERROR("Invalid input parameter (master: %p).\n", master);
        return;
    }

    dev_set_drvdata(&master->dev, data);
}

/**
 * pdm_master_get - 获取PDM主控制器的引用
 * @master: PDM主控制器
 *
 * 返回值:
 * 指向PDM主控制器的指针，或NULL（失败）
 */
struct pdm_master *pdm_master_get(struct pdm_master *master)
{
    if (!master || !get_device(&master->dev)) {
        OSA_ERROR("Invalid input parameter or unable to get device reference (master: %p).\n", master);
        return NULL;
    }

    return master;
}

/**
 * pdm_master_put - 释放PDM主控制器的引用
 * @master: PDM主控制器
 */
void pdm_master_put(struct pdm_master *master)
{
    if (master) {
        put_device(&master->dev);
    }
}


/**
 * pdm_master_alloc - 分配PDM主控制器结构
 * @data_size: 私有数据的大小
 *
 * 返回值:
 * 指向分配的PDM主控制器的指针，或NULL（失败）
 */
struct pdm_master *pdm_master_alloc(unsigned int data_size)
{
    struct pdm_master *master;
    size_t master_size = sizeof(struct pdm_master);

    master = kzalloc(master_size + data_size, GFP_KERNEL);
    if (!master) {
        OSA_ERROR("Failed to allocate memory for pdm_master.\n");
        return NULL;
    }

    device_initialize(&master->dev);
    master->dev.release = pdm_master_dev_release;
    pdm_master_devdata_set(master, (void *)master + master_size);

    return master;
}


/**
 * pdm_master_free - 释放PDM主控制器结构
 * @master: PDM主控制器
 */
void pdm_master_free(struct pdm_master *master)
{
    if (master) {
        pdm_master_put(master);
    }
}


/**
 * pdm_master_register - 注册PDM主控制器
 * @master: PDM主控制器
 *
 * 返回值:
 * 0 - 成功
 * -EINVAL - 参数无效
 * -EBUSY - 设备已存在
 * -EEXIST - 主控制器名称已存在
 * 其他负值 - 其他错误码
 */
int pdm_master_register(struct pdm_master *master)
{
    struct pdm_master *existing_master;
    int ret;

    if (!master || !strlen(master->name)) {
        OSA_ERROR("Invalid input parameters (master: %p, name: %s).\n", master, master ? master->name : "NULL");
        return -EINVAL;
    }

    if (!pdm_master_get(master)) {
        OSA_ERROR("Unable to get reference to master %s.\n", master->name);
        return -EBUSY;
    }

    mutex_lock(&pdm_master_list_mutex_lock);
    list_for_each_entry(existing_master, &pdm_master_list, entry) {
        if (!strcmp(existing_master->name, master->name)) {
            OSA_ERROR("Master already exists: %s.\n", master->name);
            mutex_unlock(&pdm_master_list_mutex_lock);
            pdm_master_put(master);
            return -EEXIST;
        }
    }
    mutex_unlock(&pdm_master_list_mutex_lock);

    master->dev.type = &pdm_master_device_type;
    dev_set_name(&master->dev, "pdm_master_device_%s", master->name);
    ret = device_add(&master->dev);
    if (ret) {
        OSA_ERROR("Failed to add device: %s, error: %d.\n", dev_name(&master->dev), ret);
        goto err_device_put;
    }

    ret = pdm_master_cdev_add(master);
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

    mutex_lock(&master->client_list_mutex_lock);
    INIT_LIST_HEAD(&master->client_list);
    mutex_unlock(&master->client_list_mutex_lock);

    master->init_done = true;
    OSA_INFO("PDM Master Registered: %s.\n", dev_name(&master->dev));

    return 0;

err_device_unregister:
    device_unregister(&master->dev);

err_device_put:
    pdm_master_put(master);
    return ret;
}

/**
 * pdm_master_unregister - 注销PDM主控制器
 * @master: PDM主控制器
 */
void pdm_master_unregister(struct pdm_master *master)
{
    struct pdm_device *client;

    if (!master) {
        OSA_ERROR("Invalid input parameters (master: %p).\n", master);
        return;
    }

    mutex_lock(&master->client_list_mutex_lock);
    if (!list_empty(&master->client_list)){
        OSA_WARN("Not all clients removed.");
        list_for_each_entry(client, &master->client_list, entry) {
            OSA_INFO("Client Name: %s.\n", dev_name(&client->dev));
        }
    }
    mutex_unlock(&master->client_list_mutex_lock);

    master->init_done = false;

    mutex_lock(&pdm_master_list_mutex_lock);
    list_del(&master->entry);
    mutex_unlock(&pdm_master_list_mutex_lock);

    mutex_lock(&master->idr_mutex_lock);
    idr_destroy(&master->device_idr);
    mutex_unlock(&master->idr_mutex_lock);

    pdm_master_cdev_delete(master);
    device_unregister(&master->dev);
    OSA_INFO("PDM Master unregistered: %s.\n", dev_name(&master->dev));
}

/**
 * pdm_master_init - 初始化PDM主控制器模块
 *
 * 返回值:
 * 0 - 成功
 * 负值 - 失败
 */
int pdm_master_init(void)
{
    int ret;

    ret = class_register(&pdm_master_class);
    if (ret < 0) {
        OSA_ERROR("Failed to register PDM Master Class, error: %d.\n", ret);
        return ret;
    }
    OSA_INFO("PDM Master Class registered.\n");

    // 初始化master驱动
    ret = pdm_subdriver_register(pdm_master_drivers, ARRAY_SIZE(pdm_master_drivers), &pdm_master_driver_list);
    if (ret < 0) {
        OSA_ERROR("Failed to register PDM Master Drivers, error: %d.\n", ret);
        return ret;
    }

    OSA_INFO("Initialize PDM Master OK.\n");
    return 0;
}

/**
 * pdm_master_exit - 卸载PDM主控制器模块
 */
void pdm_master_exit(void)
{
    pdm_subdriver_unregister(&pdm_master_driver_list);
    class_unregister(&pdm_master_class);
    OSA_INFO("PDM Master Class unregistered.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Master Module");
