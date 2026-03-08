################################################################################
#
#  build_kernel_config.sh
#
#  Copyright (c) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

KERNEL_SUBPATH="kernel/mediatek/mt8696/4.14"
DEFCONFIG_NAME="kara_defconfig"
TARGET_ARCH="arm"
TOOLCHAIN_REPO="https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9"
TOOLCHAIN_PREFIX="arm-linux-androideabi-"
MAKE_DTBS=y
KERNEL_IMAGES="arch/arm/boot/Image:arch/arm/boot/zImage:arch/arm/boot/zImage-dtb"
################################################################################
# NOTE: You must fill in the following with the path to a copy of
#       arm-linux-androideabi-4.9 compiler.
################################################################################
CROSS_COMPILER_PATH=""
