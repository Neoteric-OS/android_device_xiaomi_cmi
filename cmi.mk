#
# Copyright (C) 2024 The Android Open Source Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from star device
$(call inherit-product, device/xiaomi/cmi/device.mk)

# Inherit from the Neoteric configuration.
$(call inherit-product, vendor/neoteric/target/product/neoteric-target.mk)

# Call the MiuiCamera setup
$(call inherit-product, vendor/xiaomi/cmi-miuicamera/products/miuicamera.mk)

PRODUCT_NAME := cmi
PRODUCT_DEVICE := cmi
PRODUCT_BRAND := Xiaomi
PRODUCT_MODEL := Mi 10 Pro
PRODUCT_MANUFACTURER := Xiaomi

TARGET_BOOT_ANIMATION_RES := 1080

PRODUCT_GMS_CLIENTID_BASE := android-xiaomi
