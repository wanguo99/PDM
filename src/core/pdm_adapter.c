#include "linux/compat.h"

#include "pdm.h"

/**
 * @brief 显示所有已注册的 PDM 设备列表
 *
 * 该函数用于显示当前已注册的所有 PDM 设备的名称。
 * 如果主设备未初始化，则会返回错误。
 *
 * @param adapter PDM主控制器
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_client_show(struct pdm_adapter *adapter)
{
    struct pdm_device *client;
    int index = 1;

    if (!adapter) {
        OSA_ERROR("Adapter is not initialized.\n");
        return -ENODEV;
    }

    OSA_INFO("-------------------------\n");
    OSA_INFO("Device List:\n");

    mutex_lock(&adapter->client_list_mutex_lock);
    list_for_each_entry(client, &adapter->client_list, entry) {
        OSA_INFO("  [%d] Client Name: %s.\n", index++, client->name);
    }
    mutex_unlock(&adapter->client_list_mutex_lock);

    OSA_INFO("-------------------------\n");

    return 0;
}

/**
 * @brief 分配PDM设备ID
 *
 * 该函数用于分配一个唯一的ID给PDM设备。
 *
 * @param adapter PDM主控制器
 * @param pdmdev PDM设备
 * @return 成功返回 0，失败返回负错误码
 */
static int pdm_client_id_alloc(struct pdm_adapter *adapter, struct pdm_device *pdmdev)
{
    int id;
    int client_index = 0;

    if (!pdmdev) {
        OSA_ERROR("Invalid input parameters.\n");
        return -EINVAL;
    }

    if (of_property_read_s32(pdmdev->physical_info.of_node, "client-index", &client_index)) {
        if (pdmdev->force_dts_id) {
            OSA_ERROR("Cannot get index from dts, force_dts_id was set\n");
            return -EINVAL;
        }
        OSA_DEBUG("Cannot get index from dts\n");
    }

    if (client_index < 0) {
        OSA_ERROR("Invalid client index: %d.\n", pdmdev->client_index);
        return -EINVAL;
    }

    mutex_lock(&adapter->idr_mutex_lock);
    id = idr_alloc(&adapter->device_idr, NULL, client_index, PDM_ADAPTER_CLIENT_IDR_END, GFP_KERNEL);
    mutex_unlock(&adapter->idr_mutex_lock);

    if (id < 0) {
        if (id == -ENOSPC) {
            OSA_ERROR("No available IDs in the range.\n");
            return -EBUSY;
        } else {
            OSA_ERROR("Failed to allocate ID: %d.\n", id);
            return id;
        }
    }

    pdmdev->client_index = id;
    return 0;
}

/**
 * @brief 释放PDM设备的ID
 *
 * 该函数用于释放PDM设备的ID。
 *
 * @param adapter PDM主控制器
 * @param pdmdev PDM设备
 */
static void pdm_client_id_free(struct pdm_adapter *adapter, struct pdm_device *pdmdev)
{
    mutex_lock(&adapter->idr_mutex_lock);
    idr_remove(&adapter->device_idr, pdmdev->client_index);
    mutex_unlock(&adapter->idr_mutex_lock);
}

static struct class pdm_client_class = {
    .name = "pdm_client",
};

/**
 * @brief 默认打开函数
 *
 * 该函数是默认的文件打开操作。
 *
 * @param inode inode 结构
 * @param filp 文件结构
 * @return 成功返回 0
 */
static int pdm_client_fops_default_open(struct inode *inode, struct file *filp)
{
    return 0;
}

/**
 * @brief 默认释放函数
 *
 * 该函数是默认的文件释放操作。
 *
 * @param inode inode 结构
 * @param filp 文件结构
 * @return 成功返回 0
 */
static int pdm_client_fops_default_release(struct inode *inode, struct file *filp)
{
    OSA_INFO("fops_default_release.\n");
    return 0;
}

/**
 * @brief 默认读取函数
 *
 * 该函数是默认的文件读取操作。
 *
 * @param filp 文件结构
 * @param buf 用户空间缓冲区
 * @param count 要读取的字节数
 * @param ppos 当前文件位置
 * @return 成功返回 0
 */
static ssize_t pdm_client_fops_default_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    OSA_INFO("fops_default_read.\n");
    return 0;
}

/**
 * @brief 默认写入函数
 *
 * 该函数是默认的文件写入操作。
 *
 * @param filp 文件结构
 * @param buf 用户空间缓冲区
 * @param count 要写入的字节数
 * @param ppos 当前文件位置
 * @return 成功返回写入的字节数
 */
static ssize_t pdm_client_fops_default_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    OSA_INFO("fops_default_write.\n");
    return count;
}

/**
 * @brief 默认ioctl函数
 *
 * 该函数是默认的ioctl操作。
 *
 * @param filp 文件结构
 * @param cmd ioctl命令
 * @param arg 命令参数
 * @return -ENOTSUPP - 不支持的ioctl操作
 */
static long pdm_client_fops_default_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    OSA_INFO("Called pdm_client_fops_default_unlocked_ioctl\n");

    return -ENOTSUPP;
}

/**
 * @brief 兼容层ioctl函数
 *
 * 该函数处理兼容层的ioctl操作。
 *
 * @param filp 文件结构
 * @param cmd ioctl命令
 * @param arg 命令参数
 * @return 返回底层unlocked_ioctl的结果
 */
static long pdm_client_fops_default_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    OSA_INFO("pdm_client_fops_default_compat_ioctl.\n");

    if (_IOC_DIR(cmd) & (_IOC_READ | _IOC_WRITE)) {
        arg = (unsigned long)compat_ptr(arg);
    }

    return filp->f_op->unlocked_ioctl(filp, cmd, arg);
}

/**
 * @brief 添加PDM主控制器字符设备
 *
 * 该函数用于注册PDM主控制器字符设备。
 *
 * @param adapter PDM主控制器
 * @return 成功返回 0，失败返回负错误码
 */
static int pdm_client_device_register(struct pdm_adapter *adapter, struct pdm_device *pdmdev)
{
    int status;

    if (!adapter || !pdmdev) {
        OSA_ERROR("Invalid input parameter.\n");
        return -EINVAL;
    }

    memset(pdmdev->name, 0, PDM_DEVICE_NAME_SIZE);
    snprintf(pdmdev->name, PDM_DEVICE_NAME_SIZE, "pdm_%s.%d", adapter->name, pdmdev->client_index);
    status = alloc_chrdev_region(&pdmdev->devno, 0, 1, pdmdev->name);
    if (status < 0) {
        OSA_ERROR("Failed to allocate char device region for %s, error: %d.\n", pdmdev->name, status);
        goto err_out;
    }

    pdmdev->fops.open = pdm_client_fops_default_open;
    pdmdev->fops.release = pdm_client_fops_default_release;
    pdmdev->fops.read = pdm_client_fops_default_read;
    pdmdev->fops.write = pdm_client_fops_default_write;
    pdmdev->fops.unlocked_ioctl = pdm_client_fops_default_unlocked_ioctl;
    pdmdev->fops.compat_ioctl =  pdm_client_fops_default_compat_ioctl;

    cdev_init(&pdmdev->cdev, &pdmdev->fops);
    pdmdev->cdev.owner = THIS_MODULE;

    status = cdev_add(&pdmdev->cdev, pdmdev->devno, 1);
    if (status < 0) {
        OSA_ERROR("Failed to add char device for %s, error: %d.\n", pdmdev->name, status);
        goto err_unregister_chrdev;
    }

    device_create(&pdm_client_class, NULL, pdmdev->devno, NULL, pdmdev->name);

    OSA_DEBUG("PDM Client %s Device Registered.\n", pdmdev->name);

    return 0;

err_unregister_chrdev:
    unregister_chrdev_region(pdmdev->devno, 1);
err_out:
    return status;
}

/**
 * @brief 删除PDM主控制器字符设备
 *
 * 该函数用于注销PDM主控制器字符设备。
 *
 * @param adapter PDM主控制器
 */
static void pdm_client_device_unregister(struct pdm_device *pdmdev)
{
    if (!pdmdev) {
        OSA_ERROR("Invalid input parameter.\n");
    }

    device_destroy(&pdm_client_class, pdmdev->devno);
    cdev_del(&pdmdev->cdev);

    if (pdmdev->devno != 0) {
        unregister_chrdev_region(pdmdev->devno, 1);
        pdmdev->devno = 0;
    }

    OSA_DEBUG("PDM Client Device Unregistered.\n");
}


/**
 * @brief 向PDM主控制器添加设备
 *
 * 该函数用于向PDM主控制器添加一个新的PDM设备。
 *
 * @param adapter PDM主控制器
 * @param pdmdev 要添加的PDM设备
 * @return 成功返回 0，参数无效返回 -EINVAL
 */
int pdm_client_add(struct pdm_adapter *adapter, struct pdm_device *pdmdev)
{
    int status;

    if (!adapter || !pdmdev) {
        OSA_ERROR("Invalid input parameters (adapter: %p, pdmdev: %p).\n", adapter, pdmdev);
        return -EINVAL;
    }

    status = pdm_client_id_alloc(adapter, pdmdev);
    if (status) {
        OSA_ERROR("Alloc id for client failed: %d\n", status);
        return status;
    }

    status = pdm_client_device_register(adapter, pdmdev);
    if (status) {
        OSA_ERROR("Register device for client %s failed: %d\n", pdmdev->name, status);
        goto err_client_id_free;
    }

    mutex_lock(&adapter->client_list_mutex_lock);
    list_add_tail(&pdmdev->entry, &adapter->client_list);
    mutex_unlock(&adapter->client_list_mutex_lock);

    return 0;

err_client_id_free:
    pdm_client_id_free(adapter, pdmdev);
    return status;
}

/**
 * @brief 从PDM主控制器删除设备
 *
 * 该函数用于从PDM主控制器中删除一个PDM设备。
 *
 * @param adapter PDM主控制器
 * @param pdmdev 要删除的PDM设备
 * @return 成功返回 0，参数无效返回 -EINVAL
 */
int pdm_client_delete(struct pdm_adapter *adapter, struct pdm_device *pdmdev)
{
    if (!adapter || !pdmdev) {
        OSA_ERROR("Invalid input parameters (adapter: %p, pdmdev: %p).\n", adapter, pdmdev);
        return -EINVAL;
    }

    mutex_lock(&adapter->client_list_mutex_lock);
    list_del(&pdmdev->entry);
    mutex_unlock(&adapter->client_list_mutex_lock);

    pdm_client_device_unregister(pdmdev);
    pdm_client_id_free(adapter, pdmdev);

    OSA_DEBUG("Device %s removed from %s adapter.\n", dev_name(&pdmdev->dev), adapter->name);
    return 0;
}

/**
 * @brief PDM 主设备列表
 *
 * 该列表用于存储所有注册的 PDM 主设备。
 */
static struct list_head pdm_adapter_device_list;

/**
 * @brief 保护 PDM 主设备列表的互斥锁
 *
 * 该互斥锁用于同步对 PDM 主设备列表的访问，防止并发访问导致的数据竞争。
 */
static struct mutex pdm_adapter_device_list_mutex_lock;


/**
 * @brief 分配PDM主控制器结构
 *
 * 该函数用于分配PDM主控制器结构及其私有数据。
 *
 * @param data_size 私有数据的大小
 * @return 指向分配的PDM主控制器的指针，或NULL（失败）
 */
struct pdm_adapter *pdm_adapter_alloc(unsigned int data_size)
{
    struct pdm_adapter *adapter;
    size_t adapter_size = sizeof(struct pdm_adapter);

    adapter = kzalloc(adapter_size + data_size, GFP_KERNEL);
    if (!adapter) {
        OSA_ERROR("Failed to allocate memory for pdm_adapter.\n");
        return NULL;
    }

    INIT_LIST_HEAD(&adapter->client_list);
    mutex_init(&adapter->client_list_mutex_lock);
    adapter->data = (void *)adapter + adapter_size;

    return adapter;
}

/**
 * @brief 释放PDM主控制器结构
 *
 * 该函数用于释放PDM主控制器结构。
 *
 * @param adapter PDM主控制器
 */
void pdm_adapter_free(struct pdm_adapter *adapter)
{
    if (!adapter) {
        kfree(adapter);
    }
}

/**
 * @brief 注册 PDM 主控制器
 *
 * 该函数用于注册 PDM 主控制器，并将其添加到主控制器列表中。
 *
 * @param adapter PDM 主控制器指针
 * @return 0 - 成功
 *         -EINVAL - 参数无效
 *         -EEXIST - 主控制器名称已存在
 *         其他负值 - 其他错误码
 */
int pdm_adapter_register(struct pdm_adapter *adapter, const char* name)
{
    struct pdm_adapter *existing_adapter;

    if (!adapter || !strlen(adapter->name)) {
        OSA_ERROR("Invalid input parameters (adapter: %p, name: %s).\n", adapter, adapter ? adapter->name : "NULL");
        return -EINVAL;
    }

    strncpy(adapter->name, name, strlen(name));
    mutex_lock(&pdm_adapter_device_list_mutex_lock);
    list_for_each_entry(existing_adapter, &pdm_adapter_device_list, entry) {
        if (!strcmp(existing_adapter->name, adapter->name)) {
            OSA_ERROR("Adapter already exists: %s.\n", adapter->name);
            mutex_unlock(&pdm_adapter_device_list_mutex_lock);
            return -EEXIST;
        }
    }
    mutex_unlock(&pdm_adapter_device_list_mutex_lock);

    mutex_lock(&pdm_adapter_device_list_mutex_lock);
    list_add_tail(&adapter->entry, &pdm_adapter_device_list);
    mutex_unlock(&pdm_adapter_device_list_mutex_lock);

    mutex_init(&adapter->idr_mutex_lock);
    idr_init(&adapter->device_idr);

    adapter->init_done = true;
    OSA_DEBUG("PDM Adapter Registered: %s.\n", adapter->name);

    return 0;
}

/**
 * @brief 注销 PDM 主控制器
 *
 * 该函数用于注销 PDM 主控制器，并从主控制器列表中移除。
 *
 * @param adapter PDM 主控制器指针
 */
void pdm_adapter_unregister(struct pdm_adapter *adapter)
{
    if (!adapter) {
        OSA_ERROR("Invalid input parameters (adapter: %p).\n", adapter);
        return;
    }

    OSA_INFO("PDM Adapter unregistered: %s.\n", adapter->name);
    adapter->init_done = false;

    mutex_lock(&pdm_adapter_device_list_mutex_lock);
    list_del(&adapter->entry);
    mutex_unlock(&pdm_adapter_device_list_mutex_lock);
}

/**
 * @brief 初始化 PDM 主控制器模块
 *
 * 该函数用于初始化 PDM 主控制器模块，包括注册类和驱动。
 *
 * @return 0 - 成功
 *         负值 - 失败
 */
int pdm_adapter_init(void)
{
    int status;

    INIT_LIST_HEAD(&pdm_adapter_device_list);
    mutex_init(&pdm_adapter_device_list_mutex_lock);

    status = pdm_adapters_register();
    if (status < 0) {
        OSA_ERROR("Failed to register PDM Adapter Drivers, error: %d.\n", status);
        return status;
    }

    OSA_DEBUG("Initialize PDM Adapter OK.\n");
    return 0;
}

/**
 * @brief 卸载 PDM 主控制器模块
 *
 * 该函数用于卸载 PDM 主控制器模块，包括注销类和驱动。
 */
void pdm_adapter_exit(void)
{
    pdm_adapters_unregister();
    OSA_DEBUG("PDM Adapter Exited.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Adapter Module");
