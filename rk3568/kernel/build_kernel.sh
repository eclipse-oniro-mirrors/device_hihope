#!/bin/bash

# Copyright (c) 2021 HiHope Open Source Organization .
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

pushd ${1}
ROOT_DIR=${5}
export PRODUCT_PATH=${4}

KERNEL_SRC_TMP_PATH=${ROOT_DIR}/out/kernel/src_tmp/linux-5.10
KERNEL_SOURCE=${ROOT_DIR}/kernel/linux/linux-5.10
KERNEL_PATCH=${ROOT_DIR}/kernel/linux/patches/linux-5.10/rk3568_patch/kernel.patch
HDF_PATCH=${ROOT_DIR}/kernel/linux/patches/linux-5.10/rk3568_patch/hdf.patch
KERNEL_CONFIG_FILE=${ROOT_DIR}/kernel/linux/config/linux-5.10/arch/arm64/configs/rk3568_standard_defconfig

rm -rf ${KERNEL_SRC_TMP_PATH}
mkdir -p ${KERNEL_SRC_TMP_PATH}

cp -arf ${KERNEL_SOURCE}/* ${KERNEL_SRC_TMP_PATH}/

cd ${KERNEL_SRC_TMP_PATH}

#合入HDF patch
bash ${ROOT_DIR}/drivers/adapter/khdf/linux/patch_hdf.sh ${ROOT_DIR} ${KERNEL_SRC_TMP_PATH} ${HDF_PATCH}

#合入kernel patch
patch -p1 < ${KERNEL_PATCH}

cp -rf ${3}/kernel/logo* ${KERNEL_SRC_TMP_PATH}/

#拷贝config
cp -rf ${KERNEL_CONFIG_FILE} ${KERNEL_SRC_TMP_PATH}/arch/arm64/configs/rockchip_linux_defconfig

#编译内核
./make-ohos.sh TB-RK3568X0 > kernel_build.log 2>&1

mkdir -p ${2}
cp boot_linux.img ${2}/boot_linux_5_10.img
cp ${3}/loader/* ${2}
popd
