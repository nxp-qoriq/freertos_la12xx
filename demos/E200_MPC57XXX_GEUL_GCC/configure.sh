#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022 NXP

ccmake -DCMAKE_TOOLCHAIN_FILE="../../arch/powerpc/e200/e200gcc.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .
