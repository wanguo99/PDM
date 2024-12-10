# 指定要编译的目标模块
MODULE_NAME := pdm
obj-m := $(MODULE_NAME).o

# 定义要编译的目标模块对象文件
$(MODULE_NAME)-objs := 	src/core/pdm.o \
						src/core/pdm_device.o \
						src/core/pdm_adapter.o \
						src/core/pdm_subdriver.o

# pdm hw_driver manager
$(MODULE_NAME)-objs += 	src/hw_driver/pdm_hw_drivers.o

# pdm hw_driver
$(MODULE_NAME)-objs += 	src/hw_driver/pdm_hw_driver_i2c.o \
						src/hw_driver/pdm_hw_driver_platform.o \
						src/hw_driver/pdm_hw_driver_spi.o

# pdm adapter manager
$(MODULE_NAME)-objs +=	src/adapter/pdm_adapters.o

# pdm led
$(MODULE_NAME)-objs +=	src/adapter/led/pdm_led.o \
						src/adapter/led/pdm_led_gpio.o \
						src/adapter/led/pdm_led_pwm.o

# 添加头文件路径
ccflags-y += 	-I$(src)/include \
				-I$(src)/include/core \
				-I$(src)/include/osa \
				-I$(src)/include/uapi \
				-I$(src)/src/adapter \
				-I$(src)/src/hw_driver


# 指定额外的头文件和符号表文件
# OSA_DIR ?= $(src)/../osa
# KBUILD_EXTRA_SYMBOLS := $(OSA_DIR)/Module.symvers
