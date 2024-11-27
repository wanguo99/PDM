# 指定要编译的目标模块
MODULE_NAME := pdm
obj-m := $(MODULE_NAME).o

# 定义要编译的目标模块对象文件
$(MODULE_NAME)-objs := 	src/core/pdm_core.o \
						src/core/pdm_device.o \
						src/core/pdm_master.o \
						src/core/pdm_driver_manager.o

$(MODULE_NAME)-objs += 	src/device/pdm_device_i2c.o \
						src/device/pdm_device_platform.o \
						src/device/pdm_device_spi.o

$(MODULE_NAME)-objs += src/master/template/pdm_master_template.o 

$(MODULE_NAME)-objs += src/master/led/pdm_master_led.o 

# 添加头文件路径
ccflags-y += 	-I$(src)/include \
				-I$(src)/include/master


# 指定额外的头文件和符号表文件
# OSA_DIR ?= $(src)/../osa
# KBUILD_EXTRA_SYMBOLS := $(OSA_DIR)/Module.symvers
