#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2022 NXP

echo "$1 $2 $3 $4 $5"

echo "#######################################################################################################################"
echo "#Building FreeRTOS Image for target_model=$1, log_level=$2 build_variant=$3 boot_mode=$4 features=$5#"
echo "#######################################################################################################################"

#check feature define presence
if [ "$5" ]
then
	ENABLE_FEATURES=`echo $5 | sed 's/:/ -D/g'`
	echo "ENABLE_FEATURES = $ENABLE_FEATURES"
fi

if [ "$1" = "cfp" -o "$1" = "la1246" -o "$1" = "la1224" -o "$1" = "la1224cpe" -o "$1" = "la1238rdb" -o "$1" = "la1238cpe" ]
then
    cmake -DCMAKE_TOOLCHAIN_FILE="../../arch/powerpc/e200/e200gcc.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLA12XX_TARGET_MODEL:STRING=$1 -DLA12XX_LOG_LEVEL:STRING=$2 -DLA12XX_BUILD_VARIANT:STRING=$3 -DLA12XX_BOOT_MODE:STRING=$4 $ENABLE_FEATURES .
else
    cmake -DCMAKE_TOOLCHAIN_FILE="../../arch/powerpc/e200/e200gcc.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLA12XX_LOG_LEVEL:STRING=$2 -DLA12XX_BUILD_VARIANT:STRING=$3 -DLA12XX_BOOT_MODE:STRING=$4 $ENABLE_FEATURES .
fi

make -j4
