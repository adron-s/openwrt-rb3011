# Copyright (c) 2014 The Linux Foundation. All rights reserved.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Device/Default
	PROFILES := Default
	KERNEL_DEPENDS = $$(wildcard $(DTS_DIR)/$$(DEVICE_DTS).dts)
	KERNEL_LOADADDR = 0x42208000
	DEVICE_DTS = $$(SOC)-$(lastword $(subst _, ,$(1)))
	DEVICE_DTS_CONFIG := config@1
	IMAGES := sysupgrade.bin
	IMAGE/sysupgrade.bin = sysupgrade-tar | append-metadata
	IMAGE/sysupgrade.bin/squashfs :=
endef

define Device/LegacyImage
	KERNEL_SUFFIX := -uImage
	KERNEL = kernel-bin | append-dtb | uImage none
	KERNEL_NAME := zImage
endef

define Device/dsa-migration
  DEVICE_COMPAT_VERSION := 1.1
  DEVICE_COMPAT_MESSAGE := Config cannot be migrated from swconfig to DSA
endef

define Build/ipq-aux-loader-common
	rm -rf $@.src
	$(MAKE) -C ipq-aux-loader \
		PKG_BUILD_DIR="$@.src" \
		TARGET_DIR="$(dir $@)" LOADER_NAME="$(notdir $@)" \
		TEXT_BASE2="0x00000000" \
		BOARD="$(BOARDNAME)" PLATFORM="$(PLATFORM)" \
		BLOCKSIZE="$(BLOCKSIZE)" PAGESIZE="$(PAGESIZE)" \
		LEBSIZE="$(AUX_LOADER_LEBSIZE)" LEBCOUNT="$(AUX_LOADER_LEBCOUNT)" \
		UART="$(AUX_LOADER_UART)"	RB3011_PREINITS="$(AUX_LOADER_RB3011_PREINITS)"\
		$(1) LOADER_TYPE=$(2) compile loader.$(2)
	mv "$@.$(2)" "$@"
	rm -rf $@.src
endef

define Build/ipq-aux-loader-kernel
	$(call Build/ipq-aux-loader-common,LOADER_DATA="$@",$(1))
endef

define Device/FitImage
	KERNEL_SUFFIX := -fit-uImage.itb
	KERNEL = kernel-bin | gzip | fit gzip $$(DTS_DIR)/$$(DEVICE_DTS).dtb
	KERNEL_NAME := Image
endef

define Device/FitImageLzma
	KERNEL_SUFFIX := -fit-uImage.itb
	KERNEL = kernel-bin | lzma | fit lzma $$(DTS_DIR)/$$(DEVICE_DTS).dtb
	KERNEL_NAME := Image
endef

define Device/UbiFit
	KERNEL_IN_UBI := 1
	IMAGES := nand-factory.bin nand-sysupgrade.bin
	IMAGE/nand-factory.bin := append-ubi
	IMAGE/nand-sysupgrade.bin := sysupgrade-tar | append-metadata
endef

include $(SUBTARGET).mk

$(eval $(call BuildImage))
