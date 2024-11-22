#include <linux/platform_device.h>

#include "pdm.h"
#include "pdm_template.h"


static int pdm_template_gpio_read(int addr, int *value)
{
    return 0;
}

static int pdm_template_pwm_read(int addr, int *value)
{
    return 0;
}

static int pdm_template_uart_read(int addr, int *value)
{
    return 0;
}

static int pdm_template_adc_read(int addr, int *value)
{
    return 0;
}

static int pdm_template_dac_read(int addr, int *value)
{
    return 0;
}

static struct pdm_template_device_priv pdm_device_template_gpio_data = {
    .ops.read_reg = pdm_template_gpio_read,
};

static struct pdm_template_device_priv pdm_device_template_pwm_data = {
    .ops.read_reg = pdm_template_pwm_read,
};

static struct pdm_template_device_priv pdm_device_template_uart_data = {
    .ops.read_reg = pdm_template_uart_read,
};

static struct pdm_template_device_priv pdm_device_template_adc_data = {
    .ops.read_reg = pdm_template_adc_read,
};

static struct pdm_template_device_priv pdm_device_template_dac_data = {
    .ops.read_reg = pdm_template_dac_read,
};


static int pdm_template_platform_probe(struct platform_device *pdev)
{
    const struct pdm_template_device_priv *data;
    struct pdm_device *pdmdev;
    const char *compatible;
    int ret;

    pdmdev = pdm_device_alloc(sizeof(struct pdm_template_device_priv));
    if (!pdmdev) {
        OSA_ERROR("Failed to allocate pdm_device.\n");
        return -ENOMEM;
    }


    ret = of_property_read_string(pdev->dev.of_node, "compatible", &compatible);
    if (ret) {
        pr_err("Failed to read compatible property: %d\n", ret);
        goto unregister_pdmdev;
    }

    strcpy(pdmdev->compatible, compatible);
    pdmdev->real_device = pdev;
    ret = pdm_template_master_register_device(pdmdev);
    if (ret) {
        OSA_ERROR("Failed to add template device, ret=%d.\n", ret);
        goto free_pdmdev;
    }

	data = of_device_get_match_data(&pdev->dev);
	if (!data)
	{
        OSA_ERROR("Failed to get match data, ret=%d.\n", ret);
		goto unregister_pdmdev;
    }
    pdm_device_devdata_set(pdmdev, (void *)data);

    OSA_INFO("Template PLATFORM Device Probed.\n");
    return 0;

unregister_pdmdev:
    pdm_template_master_unregister_device(pdmdev);

free_pdmdev:
    pdm_device_free(pdmdev);

    return ret;

}

static int pdm_template_platform_remove(struct platform_device *pdev)
{
    struct pdm_device *pdmdev = pdm_template_master_find_pdmdev(pdev);
    if (NULL == pdmdev) {
        OSA_ERROR("Failed to find pdm device from master.\n");
        return -EINVAL;
    }

    pdm_template_master_unregister_device(pdmdev);
    pdm_device_free(pdmdev);

    OSA_INFO("Template PLATFORM Device Removed.\n");
    return 0;
}


/**
  * @ dts节点配置示例

 *  / {
 *      model = "Freescale i.MX6 UltraLiteLite 14x14 EVK Board";
 *      compatible = "fsl,imx6ull-14x14-evk", "fsl,imx6ull";

 *      template-platform-0 {
 *          compatible = "pdm,template-platform";
 *          status = "okay";
 *      };
 *  };
*/
static const struct of_device_id of_platform_platform_match[] = {
	{ .compatible = "pdm,template-gpio", .data = &pdm_device_template_gpio_data, },
	{ .compatible = "pdm,template-pwm",  .data = &pdm_device_template_pwm_data, },
	{ .compatible = "pdm,template-uart", .data = &pdm_device_template_uart_data, },
	{ .compatible = "pdm,template-adc",  .data = &pdm_device_template_adc_data, },
	{ .compatible = "pdm,template-dac",  .data = &pdm_device_template_dac_data, },
	{},
};
MODULE_DEVICE_TABLE(of, of_platform_platform_match);

static struct platform_driver pdm_template_platform_driver = {
	.probe		= pdm_template_platform_probe,
	.remove	= pdm_template_platform_remove,
	.driver		= {
		.name	= "pdm-template-platform",
		.of_match_table = of_platform_platform_match,
	},
};


/**
 * @brief 初始化 PLATFORM 驱动
 *
 * 该函数用于初始化 PLATFORM 驱动，注册 PLATFORM 驱动到系统。
 *
 * @return 成功返回 0，失败返回负错误码
 */
int pdm_template_platform_driver_init(void) {
    int ret;

    ret = platform_driver_register(&pdm_template_platform_driver);
    if (ret) {
        OSA_ERROR("Failed to register Template PLATFORM Driver.\n");
        return ret;
    }
    OSA_INFO("Template PLATFORM Driver Initialized.\n");
    return 0;
}

/**
 * @brief 退出 PLATFORM 驱动
 *
 * 该函数用于退出 PLATFORM 驱动，注销 PLATFORM 驱动。
 */
void pdm_template_platform_driver_exit(void) {
    platform_driver_unregister(&pdm_template_platform_driver);
    OSA_INFO("Template PLATFORM Driver Exited.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("<guohaoprc@163.com>");
MODULE_DESCRIPTION("PDM Template PLATFORM Driver");
