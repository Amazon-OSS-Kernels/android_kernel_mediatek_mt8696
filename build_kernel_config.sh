################################################################################
#
#  build_kernel_config.sh
#
#  Copyright (c) 2023 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

KERNEL_SUBPATH="kernel/mediatek/mt8696/5.10"
DEFCONFIG_NAME="defconfig gki_defconfig karat.config"
TARGET_ARCH="arm64"
TOOLCHAIN_REPO="https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9"
TOOLCHAIN_BRANCH="llvm-r383902b"
TOOLCHAIN_NAME="aarch64-linux-android-4.9"
TOOLCHAIN_PREFIX="aarch64-linux-android-"
PARALLEL_EXECUTION="-j16"

# Expected image files are seperated with ":"
KERNEL_IMAGES="arch/arm64/boot/Image:arch/arm64/boot/Image.gz"

################################################################################
# NOTE: You must fill in the following with the path to a copy of Clang compiler
# clang-r416183b
################################################################################
CLANG_COMPILER_PATH=""
