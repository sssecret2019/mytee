TEE_SDK_DIR=$(shell pwd)
include ${TEE_SDK_DIR}/config.mk

TEE_SDK_DIR = $(shell pwd)

all: arm-tf-final optee-client-final optee-examples-final mytee-examples-final linux-final boot-final

################################################################################
# ARM Trust Firmware
################################################################################
ARM_TF_FLAGS ?= \
	NEED_BL32=yes \
	BL32=${TEE_SDK_DIR}/optee_os/out/arm/core/tee-header_v2.bin \
	BL32_EXTRA1=${TEE_SDK_DIR}/optee_os/out/arm/core/tee-pager_v2.bin \
	BL32_EXTRA2=${TEE_SDK_DIR}/optee_os/out/arm/core/tee-pageable_v2.bin \
	BL33=${TEE_SDK_DIR}/u-boot/u-boot.bin \
	DEBUG=1 \
	V=0 \
	CRASH_REPORTING=1 \
	LOG_LEVEL=40 \
	PLAT=rpi3 \
	SPD=opteed
ARM_TF_PATH ?= ./arm-trusted-firmware
.PHONY: arm-tf
arm-tf: optee-os u-boot-env
	CROSS_COMPILE=${CROSS_COMPILE_AARCH64} $(MAKE) -C ${TEE_SDK_DIR}/arm-trusted-firmware $(ARM_TF_FLAGS) all fip

.PHONY: arm-tf-final
arm-tf-final: arm-tf
	cp ${TEE_SDK_DIR}/arm-trusted-firmware/build/rpi3/debug/armstub8.bin ./out/boot/

.PHONY: arm-tf-clean
arm-tf-clean:
	$(MAKE) -C $(ARM_TF_PATH) $(ARM_TF_FLAGS) clean

################################################################################
# Das U-Boot
################################################################################
U-BOOT_DEFCONFIG_COMMON_FILES := \
		${TEE_SDK_DIR}/u-boot/configs/rpi_3_32b_defconfig \
		${TEE_SDK_DIR}/firmware/u-boot_rpi3.conf
.PHONY: u-boot
u-boot: u-boot-defconfig
	ARCH=arm $(MAKE) -C ./u-boot all
	ARCH=arm $(MAKE) -C ./u-boot tools

.PHONY: u-boot-clean
u-boot-clean: u-boot-defconfig-clean
	ARCH=arm $(MAKE) -C ./u-boot clean

.PHONY: u-boot-env
u-boot-env: ${TEE_SDK_DIR}/firmware/uboot.env.txt u-boot
	./u-boot/tools/mkenvimage -s 0x4000 -o ./out/boot/uboot.env ${TEE_SDK_DIR}/firmware/uboot.env.txt

.PHONY: u-boot-env-clean
u-boot-env-clean:
	rm -f ${TEE_SDK_DIR}/uboot.env

.PHONY: u-boot-defconfig
u-boot-defconfig:$(U-BOOT_DEFCONFIG_COMMON_FILES)
	cd ${TEE_SDK_DIR}/u-boot && \
		ARCH=arm \
		scripts/kconfig/merge_config.sh $(U-BOOT_DEFCONFIG_COMMON_FILES)

.PHONY: u-boot-defconfig-clean
u-boot-defconfig-clean:
	rm -f ${TEE_SDK_DIR}/u-boot/.config

################################################################################
# OP-TEE os
################################################################################
OPTEE_OS_FLAGS ?= \
	PLATFORM=rpi3 \
	O=out/arm CFG_ARM32_core=y \
	CROSS_COMPILE=$(CROSS_COMPILE) \
	CROSS_COMPILE_core=$(CROSS_COMPILE) \
	CROSS_COMPILE_ta_arm32=$(CROSS_COMPILE) \
	CFG_TEE_CORE_LOG_LEVEL=4 \
	DEBUG=0 \
	CFG_TEE_BENCHMARK=n

.PHONY: optee-os
optee-os:
	mkdir -p out
	mkdir -p out/boot
	mkdir -p out/rootfs
	mkdir -p out/rootfs/bin
	mkdir -p out/rootfs/lib
	mkdir -p out/rootfs/lib/optee_armtz
	$(MAKE) -C ${TEE_SDK_DIR}/optee_os $(OPTEE_OS_FLAGS)

OPTEE_OS_CLEAN_FLAGS ?= O=out/arm CFG_ARM32_core=y CFG_mytee_fb=y

.PHONY: optee-os-clean
optee-os-clean:
	$(MAKE) -C ${TEE_SDK_DIR}/optee_os $(OPTEE_OS_CLEAN_FLAGS) clean

################################################################################
# OP-TEE client
################################################################################
OPTEE_CLIENT_FLAGS ?= CROSS_COMPILE=$(CROSS_COMPILE) \
	CFG_TEE_BENCHMARK=n \

.PHONY: optee-client
optee-client:
	$(MAKE) -C ${TEE_SDK_DIR}/optee_client $(OPTEE_CLIENT_FLAGS)

.PHONY: optee-client-final
optee-client-final: optee-client
	cp ${TEE_SDK_DIR}/optee_client/out/tee-supplicant/tee-supplicant ./out/rootfs/bin/
	cp -d ${TEE_SDK_DIR}/optee_client/out/libteec/libteec.so* ./out/rootfs/lib/

.PHONY: optee-client-clean
optee-client-clean:
	$(MAKE) -C ${TEE_SDK_DIR}/optee_client $(OPTEE_CLIENT_CLEAN_FLAGS) \
clean


clean: arm-tf-clean u-boot-clean optee-os-clean optee-client-clean
.PHONY: clean

install:
.PHONY: install

################################################################################
# OP-TEE examples
################################################################################
.PHONY: optee-examples
optee-examples:
	$(MAKE) -C ${TEE_SDK_DIR}/optee_examples \
HOST_CROSS_COMPILE=${CROSS_COMPILE} \
TEEC_EXPORT=${TEE_SDK_DIR}/optee_client/out/export \
TA_DEV_KIT_DIR=${TEE_SDK_DIR}/optee_os/out/arm/export-ta_arm32

.PHONY: optee-examples-final
optee-examples-final: optee-examples
	cp ${TEE_SDK_DIR}/optee_examples/acipher/host/optee_example_acipher ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/optee_examples/aes/host/optee_example_aes ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/optee_examples/hello_world/host/optee_example_hello_world ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/optee_examples/hotp/host/optee_example_hotp ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/optee_examples/random/host/optee_example_random ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/optee_examples/secure_storage/host/optee_example_secure_storage ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/optee_examples/xset_test/host/xset_test ./out/rootfs/bin/
	
	cp ${TEE_SDK_DIR}/optee_examples/acipher/ta/*.ta ./out/rootfs/lib/optee_armtz/
	cp ${TEE_SDK_DIR}/optee_examples/aes/ta/*.ta ./out/rootfs/lib/optee_armtz/
	cp ${TEE_SDK_DIR}/optee_examples/hello_world/ta/*.ta ./out/rootfs/lib/optee_armtz/
	cp ${TEE_SDK_DIR}/optee_examples/hotp/ta/*.ta ./out/rootfs/lib/optee_armtz/
	cp ${TEE_SDK_DIR}/optee_examples/random/ta/*.ta ./out/rootfs/lib/optee_armtz/
	cp ${TEE_SDK_DIR}/optee_examples/secure_storage/ta/*.ta ./out/rootfs/lib/optee_armtz/
	
################################################################################
# MYTEE examples
################################################################################
.PHONY: mytee-examples
optee-examples:
	$(MAKE) -C ${TEE_SDK_DIR}/mytee_examples \
HOST_CROSS_COMPILE=${CROSS_COMPILE} \
TEEC_EXPORT=${TEE_SDK_DIR}/optee_client/out/export \
TA_DEV_KIT_DIR=${TEE_SDK_DIR}/optee_os/out/arm/export-ta_arm32

.PHONY: mytee-examples-final
mytee-examples-final: mytee-examples
	cp ${TEE_SDK_DIR}/mytee_examples/myta_fb_write/host/myta_fb_write ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/mytee_examples/myta_fb_mmap/host/myta_fb_mmap ./out/rootfs/bin/ 
	cp ${TEE_SDK_DIR}/mytee_examples/myta_tpm/host/myta_tpm ./out/rootfs/bin/
	cp ${TEE_SDK_DIR}/mytee_examples/myta_keyboard/host/myta_keyboard ./out/rootfs/bin/

################################################################################
# linux
################################################################################
.PHONY: linux-config
linux-config:
	ARCH=arm $(MAKE) -C ${TEE_SDK_DIR}/linux bcm2709_defconfig

.PHONY: linux-build
linux-build: linux-config
	ARCH=arm $(MAKE) -C ${TEE_SDK_DIR}/linux zImage dtbs modules

.PHONY: linux-uimage
linux-uimage: linux-build
	mkimage -A arm -O linux -T kernel -C none -a 0x02000000 -e 0x02000000 -n "linux kernel image" -d ${TEE_SDK_DIR}/linux/arch/arm/boot/zImage ${TEE_SDK_DIR}/linux/arch/arm/boot/uImage

.PHONY: linux-final
linux-final: linux-uimage
	ARCH=arm $(MAKE) -C ${TEE_SDK_DIR}/linux modules_install INSTALL_MOD_PATH=${TEE_SDK_DIR}/out/rootfs/
	cp ${TEE_SDK_DIR}/linux/arch/arm/boot/uImage ./out/boot/
	cp ${TEE_SDK_DIR}/linux/arch/arm/boot/dts/bcm2710-rpi-3-b-plus.dtb ./out/boot/
	cp ${TEE_SDK_DIR}/linux/arch/arm/boot/dts/bcm2710-rpi-3-b.dtb ./out/boot/

.PHONY: boot-final
boot-final:
	cp ${TEE_SDK_DIR}/firmware/config.txt ./out/boot/

.PHONY: patch
patch: atf-patch linux-patch

.PHONY: atf-patch
atf-patch:
	patch ${TEE_SDK_DIR}/arm-trusted-firmware/plat/rpi3/platform.mk ${TEE_SDK_DIR}/patch/platform.mk.patch

.PHONY: linux-patch
linux-patch:
	patch ${TEE_SDK_DIR}/linux/arch/arm/boot/dts/bcm2710.dtsi ${TEE_SDK_DIR}/patch/bcm2710.dtsi.patch
	patch ${TEE_SDK_DIR}/linux/arch/arm/configs/bcm2709_defconfig ${TEE_SDK_DIR}/patch/bcm2709_defconfig.patch
	patch ${TEE_SDK_DIR}/linux/arch/arm/boot/dts/bcm270x.dtsi ${TEE_SDK_DIR}/patch/bcm270x.dtsi.patch
	patch ${TEE_SDK_DIR}/linux/arch/arm/boot/dts/bcm2710-rpi-3-b.dts ${TEE_SDK_DIR}/patch/bcm2710-rpi-3-b.dts.patch
	patch ${TEE_SDK_DIR}/linux/arch/arm/boot/dts/bcm2710-rpi-3-b-plus.dts ${TEE_SDK_DIR}/patch/bcm2710-rpi-3-b-plus.dts.patch
