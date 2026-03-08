################################################################################
#
#  build_uboot_config.sh
#
#  Copyright (c) 2023 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

UBOOT_SUBPATH="bootable/bootloader/lk-mtk/mt8696"
UBOOT_DEFCONFIG_NAME="karat"
export CROSS_COMPILE_STANDALONE=yes

# Expected image files are seperated with ":"
UBOOT_IMAGES="build/u-boot.bin"

################################################################################
# NOTE: You must fill in the following with the path to a copy of an
#       gcc-linaro-aarch64-none-elf-4.8-2013.11_linux (aarch64-none-elf compiler) and
#       riscv-none-gcc/7.2.0-4-20180606-1631
################################################################################
CROSS_COMPILER_PATH=""
RISCV_COMPILER_PATH=""
