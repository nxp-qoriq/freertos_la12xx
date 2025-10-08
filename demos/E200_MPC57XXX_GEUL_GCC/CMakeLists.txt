# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2025 NXP

INCLUDE(CMakeForceCompiler)
INCLUDE(CMakeDependentOption)

# CROSS COMPILER SETTING
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12)
PROJECT(LA12XX)

# THE VERSION NUMBER
SET (Tutorial_VERSION_MAJOR 1)
SET (Tutorial_VERSION_MINOR 0)

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

#set(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# CURRENT DIRECTORY
SET(ProjDirPath ${CMAKE_CURRENT_SOURCE_DIR})

# RELEASE LINK FILE
add_definitions( -DGEUL_E200_MASTER_CORE=0 )
SET(LA12XX_TARGET_MODEL la1224 CACHE STRING "GEUL Platform")
SET_PROPERTY(CACHE LA12XX_TARGET_MODEL PROPERTY STRINGS simu cfp la1246 la1224 la1224cpe la1238rdb la1238cpe)

SET(LA12XX_BOOT_MODE pci CACHE STRING "GEUL Boot Mode")
SET_PROPERTY(CACHE LA12XX_BOOT_MODE PROPERTY STRINGS pebm xspi pci)

SET(LA12XX_BUILD_VARIANT debug CACHE STRING "GEUL Build Variant")
SET_PROPERTY(CACHE LA12XX_BUILD_VARIANT PROPERTY STRINGS debug release)

SET(LA12XX_LOG_LEVEL info CACHE STRING "GEUL Log Level")
SET_PROPERTY(CACHE LA12XX_LOG_LEVEL PROPERTY STRINGS err info dbg isr all)

IF(LA12XX_TARGET_MODEL MATCHES cfp)
	SET(LA12XX_CFP ON)
ELSEIF(LA12XX_TARGET_MODEL MATCHES la1224cpe)
	SET(LA12XX_LA1224 ON)
	SET(LA12XX_LA1224CPE ON)
	SET(CPE_IPC ON)
ELSEIF(LA12XX_TARGET_MODEL MATCHES la1224)
	SET(LA12XX_LA1224 ON)
	SET(LA12XX_MTD_I2C_PWR_INFO ON)
	SET(LA12XX_MTD_I2C_DIODE_TEMP ON)
ELSEIF(LA12XX_TARGET_MODEL MATCHES la1246)
	SET(LA12XX_LA1246 ON)
ELSEIF(LA12XX_TARGET_MODEL MATCHES la1238rdb)
	SET(LA12XX_LA1238RDB ON)
ELSEIF(LA12XX_TARGET_MODEL MATCHES la1238cpe)
	SET(LA12XX_LA1238CPE ON)
	SET(CPE_IPC ON)
ENDIF()

# OPTION and CMAKE_DEPENDENT_OPTION can be defined as below:
# OPTION(NAME "String" Default state(ON/OFF)
# CMAKE_DEPENDENT_OPTION(NAME "String" Default state(ON/OFF) DEPENDENCY_NAME State(ON/OFF)

# Features
OPTION(LA12XX_FEATURES "Features" ON)
if(LA12XX_DRIVER_DIORA_RF_LIB OR LA12XX_DRIVER_DIORA_RF)
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_L1C_REFAPP "Enable L1C RefApp" ON "LA12XX_FEATURES;NOT LA12XX_FEATURE_RUDEMO;NOT LA12XX_FEATURE_TESTFRAMEWORK" OFF)
else()
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_L1C_REFAPP "Enable L1C RefApp" OFF "LA12XX_FEATURES;NOT LA12XX_FEATURE_RUDEMO;NOT LA12XX_FEATURE_TESTFRAMEWORK" OFF)
endif()
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_RUDEMO "Enable RU Demo" OFF "LA12XX_FEATURES;NOT LA12XX_FEATURE_L1C_REFAPP;NOT LA12XX_FEATURE_TESTFRAMEWORK" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_IDLECALIBRATE "Enable IdleCalibrate" OFF "LA12XX_FEATURES" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_TESTFRAMEWORK "Enable TestFramework" ON "LA12XX_FEATURES;NOT LA12XX_FEATURE_L1C_REFAPP;NOT LA12XX_FEATURE_RUDEMO" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_HOST_CLI "Enable CLI on MODEM" OFF "LA12XX_FEATURES;NOT LA12XX_FEATURE_TESTFRAMEWORK;NOT LA12XX_FEATURE_L1C_REFAPP;NOT LA12XX_FEATURE_RUDEMO" ON)
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_DU_ONLY "Enable DU Only Memory Optimizations" OFF "LA12XX_FEATURES;" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_WARMUP "Enable FRTOS code compilation for WARMUP feature" OFF "LA12XX_FEATURES;" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_FEATURE_MODEM_LOG_AT_HOST "Enable modem log direct to host feature" OFF "LA12XX_FEATURES;" OFF)

# Drivers
OPTION(LA12XX_DRIVERS "Drivers" ON)

#IF DIORA ENABLED

if(LA12XX_DRIVER_DIORA_RF_LIB OR LA12XX_DRIVER_DIORA_RF)
############DIORA PATH
	if (NOT "$ENV{RF_DRIVER_PATH}" STREQUAL "")
	set(RFDriverPath "$ENV{RF_DRIVER_PATH}" CACHE INTERNAL "DIORA PATH copied from environment variable")
	else()
	SET(RFDriverPath ${ProjDirPath}/../../ext/diora)
	endif()
#full build vs library based build
	if(LA12XX_DRIVER_DIORA_RF_LIB)
	CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_DIORA_RF_LIB "DIORA RF LIB Support" ON "LA12XX_DRIVERS;LA12XX_LA1224;NOT LA12XX_FEATURE_DU_ONLY" OFF)
	else ()
	CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_DIORA_RF "DIORA RF Support" ON "LA12XX_DRIVERS;LA12XX_LA1224;NOT LA12XX_FEATURE_DU_ONLY" OFF)
	endif()
	message("*** BUILD for DIORA_RF_LIB - RF_DRIVER_PATH = ${RFDriverPath}")

elseif(LA12XX_DRIVER_ICEWINGS_RF)
###############ICEWINGS RF
	if (NOT "$ENV{ICEWINGS_DIR_PATH}" STREQUAL "")
		set(RFDriverPath "$ENV{ICEWINGS_DIR_PATH}" CACHE INTERNAL "ICEWINGS PATH copied from environment variable")
	else()
		SET(RFDriverPath ${ProjDirPath}/../../ext/icewings/drivers/icewings)
	endif()
	CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_ICEWINGS_RF "IceWings RF Support" ON "LA12XX_DRIVERS;NOT LA12XX_LA1246;NOT LA12XX_FEATURE_DU_ONLY" OFF)
	message("*** BUILD for ICEWINGS - ICEWINGS_DIR_PATH = ${RFDriverPath}")

elseif(LA12XX_DRIVER_ADI_RF)
###############ADI RF
	if (NOT "$ENV{RF_DRIVER_PATH}" STREQUAL "")
		set(RFDriverPath "$ENV{RF_DRIVER_PATH}" CACHE INTERNAL "ADI RF DRIVER PATH copied from environment variable")
	else()
		SET(RFDriverPath ${ProjDirPath}/../../ext/adi)
	endif()
	CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_ADI_RF "ADI RF Support" ON "LA12XX_DRIVERS;NOT LA12XX_LA1246;NOT LA12XX_FEATURE_DU_ONLY" OFF)
	message("*** BUILD for ADI - ADI_DIR_PATH = ${RFDriverPath}")
elseif(LA12XX_LA1238RDB OR LA12XX_LA1238CPE OR LA12XX_DRIVER_PATRON_RF)
# PATRRON RF CARDS
	CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_PATRON_RF "PATRON-YUCCA RF Support" ON "LA12XX_DRIVERS;NOT LA12XX_LA1246;NOT LA12XX_FEATURE_DU_ONLY" OFF)
	SET(RFDriverPath ${ProjDirPath}/../../ext/yucca)
	message("*** BUILD for PATRON_RF (YUCCA)")
elseif(LA12XX_DRIVER_AGAVE_RF)
###############AGAVE RF
	message("*** BUILD for AGAVE_RF")
	SET(RFDriverPath ${ProjDirPath}/../../drivers/agave)
	CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_AGAVE_RF "AGAVE RF Support" ON "LA12XX_DRIVERS;LA12XX_LA1224;NOT LA12XX_FEATURE_DU_ONLY" OFF)
endif() 

CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_BBDEV_IPC "BBDEV IPC Driver" ON "LA12XX_DRIVERS;NOT LA12XX_LA1238CPE;NOT LA12XX_LA1224CPE" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_FECA "FECA Driver" ON "LA12XX_DRIVERS;NOT LA12XX_LA1238CPE" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_DCS_LS "DCS LS Driver" ON "LA12XX_DRIVERS;NOT LA12XX_FEATURE_DU_ONLY" OFF "LA12XX_FEATURE_DU_ONLY" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_AXIQ "AXIQ Driver" ON "LA12XX_DRIVERS;NOT LA12XX_FEATURE_DU_ONLY" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_TBGEN "TBGEN Driver" ON "LA12XX_DRIVERS" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_PCI "PCIE Driver" OFF "LA12XX_DRIVERS" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_PCI_LAT_FP "PCIE RC FPGA Driver" OFF "LA12XX_DRIVERS" OFF)
CMAKE_DEPENDENT_OPTION(LA12XX_DRIVER_FLEXSPI "FLEXSPI Driver" ON "LA12XX_DRIVERS;LA12XX_BUILD_VARIANT MATCHES debug;NOT LA12XX_FEATURE_DU_ONLY" OFF)

IF(LA12XX_CFP)
	add_definitions( -DGEUL_LA12XX=1 )
ELSEIF(LA12XX_LA1224)
	if(LA12XX_FEATURE_WARMUP)
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${ProjDirPath}/../../soc/la12xx/geul_e200_la1224_warmup.ld -static")
	else()
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${ProjDirPath}/../../soc/la12xx/geul_e200_la1224.ld -static")
	endif()
	add_definitions( -DGEUL_LA12XX=1 )
	add_definitions( -DGEUL_LA1224=1 )
	if(LA12XX_MTD_I2C_PWR_INFO)
		add_definitions( -DMTD_I2C_PWR_INFO=1 )
	endif()
	if(LA12XX_MTD_I2C_DIODE_TEMP)
		add_definitions( -DMTD_I2C_DIODE_TEMP=1 )
	endif()
	IF(LA12XX_LA1224CPE)
		add_definitions( -DCPE_IPC=1 )
	ENDIF()
	if(LA12XX_FEATURE_MODEM_LOG_AT_HOST)
		add_definitions( -DGEUL_LA12XX_MODEM_LOG_AT_HOST_ENABLE=1 )
	else()
		add_definitions( -DGEUL_LA12XX_MODEM_LOG_AT_HOST_ENABLE=0 )
	endif()
ELSEIF(LA12XX_LA1246)
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${ProjDirPath}/../../soc/la12xx/geul_e200_la1246.ld")
	add_definitions( -DGEUL_LA12XX=1 )
	add_definitions( -DGEUL_LA1246=1 )
ELSEIF(LA12XX_LA1238RDB)
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${ProjDirPath}/../../soc/la12xx/geul_e200_la1238rdb.ld -static")
	add_definitions( -DGEUL_LA12XX=1 )
	add_definitions( -DGEUL_LA1238RDB=1 )
ELSEIF(LA12XX_LA1238CPE)
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -T${ProjDirPath}/../../soc/la12xx/geul_e200_la1238cpe.ld -static")
	add_definitions( -DGEUL_LA12XX=1 )
	add_definitions( -DGEUL_LA1238CPE=1 )
	add_definitions( -DCPE_IPC=1 )
ENDIF()

IF(LA12XX_BOOT_MODE MATCHES pebm)
add_definitions( -DGEUL_BOOT_MODE_PEBM=1 )
ELSEIF(LA12XX_BOOT_MODE MATCHES xspi)
add_definitions( -DGEUL_BOOT_MODE_XSPI=1 )
ELSEIF(LA12XX_BOOT_MODE MATCHES pci)
add_definitions( -DGEUL_BOOT_MODE_PCI=1 )
ENDIF()
 
# SET FLAG FOR MODEM
add_definitions( -DMODEM=1 )

# Set Queue count for each QDMA Block
# 0 - 8 => Valid values
# 0 represent No queue needed for particular Block
# 8 represent Max queues
add_definitions( -DQDMA_QUEUE_COUNT_BLOCK0=8 )
add_definitions( -DQDMA_QUEUE_COUNT_BLOCK1=8 )
add_definitions( -DQDMA_QUEUE_COUNT_BLOCK2=8 )
add_definitions( -DQDMA_QUEUE_COUNT_BLOCK3=8 )

SET(QDMA_FROM_PEB_TO_FRAM ON)
if(QDMA_FROM_PEB_TO_FRAM)
	add_definitions( -DQDMA_PEB_TO_FRAM=1 )
endif()

# SET this flag when VSPA is using overlay area.
# Otherwise application will use it for for local usages.
if(LA12XX_LA1238RDB OR LA12XX_LA1238CPE)
	SET(VSPA_OV_PEBM_RESERVE ON)
else()
	SET(VSPA_OV_PEBM_RESERVE OFF)
endif()

IF(LA12XX_LOG_LEVEL MATCHES err)
	add_definitions( -DGUL_LOG_LEVEL=1 )
ELSEIF(LA12XX_LOG_LEVEL MATCHES info)
	add_definitions( -DGUL_LOG_LEVEL=2 )
ELSEIF(LA12XX_LOG_LEVEL MATCHES dbg)
	add_definitions( -DGUL_LOG_LEVEL=3 )
ELSEIF(LA12XX_LOG_LEVEL MATCHES isr)
	add_definitions( -DGUL_LOG_LEVEL=4 )
ELSEIF(LA12XX_LOG_LEVEL MATCHES all)
	add_definitions( -DGUL_LOG_LEVEL=5 )
ELSE()
	add_definitions( -DGUL_LOG_LEVEL=0 )
ENDIF()

IF(LA12XX_BUILD_VARIANT MATCHES debug)
	SET(ENABLE_L1C OFF)
	SET(RELEASE_MODE OFF)
	add_definitions( -DDCM_ARM_POLAR_DECODER_POC_EN=1 )
ELSEIF(LA12XX_BUILD_VARIANT MATCHES release)
	SET(ENABLE_L1C OFF)
	SET(ENABLE_IDLECALIBRATE OFF)
	add_definitions( -DRELEASE_MODE=1 )
	SET(RELEASE_MODE ON)
	add_definitions( -DDCM_ARM_POLAR_DECODER_POC_EN=0 )
ENDIF()

IF(LA12XX_BUILD_VARIANT MATCHES debug)
if(LA12XX_FEATURE_IDLECALIBRATE)
	add_definitions( -DCONFIG_IDLE_CALIBRATE=1 )
else()
	add_definitions( -DCONFIG_IDLE_CALIBRATE=0 )
endif()
ENDIF()

if(ENABLE_L1C)
	SET(GEUL_L1C_LIB	libl1c.a)
	SET(GEUL_L1C_LIB_PATH "${ProjDirPath}/../../lib/l1c")
	add_definitions( -DCONFIG_L1C_ENABLE=1 )
	add_definitions( -DDLSCH_HARQ  )
	add_definitions( -DASSERT_LOG_ENABLE=1 )
endif()

#Compile On/Off Hawk support
SET(ENABLE_HAWK OFF)

#Depending on the requirement, developer can enable/disable
#configs to meet feature requirement as well as text size limitation.
#e.g.: Due to text size limitation, RF Driver needs to be
#compiled-off in debug target as below.
#Similary user can enable RF and disable some other section to meet memory
#requirement when performance profiling other features
if(ENABLE_HAWK)
	add_definitions( -DHAWK_ENABLED )
	IF(LA12XX_BUILD_VARIANT MATCHES debug)
		SET(ENABLE_RF OFF)
		SET(LA12XX_DRIVER_AGAVE_RF OFF)
		SET(LA12XX_DRIVER_PATRON_RF OFF)
	ENDIF()
endif()

if(LA12XX_DRIVER_FLEXSPI)
	add_definitions( -DCONFIG_MT25QU512A=1)
	add_definitions( -DCONFIG_FLEXSPI=1)
	if(NOT (LA12XX_BUILD_VARIANT MATCHES debug AND LA12XX_LA1238RDB))
		add_definitions( -DFSPI_CMD_ENABLE=1)
	endif()
endif()

# Treat warnings as errors
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# RELEASE ASM FLAGS
SET(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE}  -mvle -mbig -mregnames -c")

# SET(CMAKE_C_FLAGS_ARCH "-mcpu=e200z7 -mhard-float ")

# RELEASE C FLAGS
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -std=c99 --sysroot=$ENV{E200GCC_DIR}/powerpc-eabivle/newlib/ -O3 -Wall -Wextra -mcpu=e200z7 -G0 -mhard-float -fno-strict-aliasing -ffunction-sections -fdata-sections -Wdouble-promotion -Wfloat-equal -Wpointer-arith -Wno-enum-compare -falign-functions=32")

# RELEASE LD FLAGS
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} --sysroot=$ENV{E200GCC_DIR}/powerpc-eabivle/newlib/ -Xlinker --gc-sections -mvle -mcpu=e200z7 -N -static")

if(RELEASE_MODE)
	# RELEASE LD FLAGS
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -specs=nosys.specs -Wl,--print-memory-usage")
else()
	# RELEASE ASM FLAGS
	SET(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE}  -g")

	# RELEASE C FLAGS
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")

	# RELEASE LD FLAGS
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -specs=nosys.specs -Wl,--print-memory-usage")
endif()

set(ENABLE_SPE OFF)

if(ENABLE_SPE)
	add_definitions(-DENABLE_SPE)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -mspe2")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -mspe2")
endif()

# ASM MACRO
SET(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG}  -D__DEBUG --sysroot=$ENV{E200GCC_DIR}/powerpc-eabivle/newlib/")

# SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -D__NDEBUG -DMODEM")
if(LA12XX_LA1224)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DFECA_ENABLED")
	#Compile on/off DSPI Normal Mode
	SET(ENABLE_NORMAL_SPI_MODE ON)
	SET(ENABLE_DSPI_STREAM_MODE ON)
	#Compile on/off SI551x support
	SET(ENABLE_SI551x ON)
	#Compile on/off SI551x CLI Command support
	SET(ENABLE_SI551x_CLI ON)
endif()

if(LA12XX_LA1246)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DFECA_ENABLED")
	#Compile on/off DSPI Normal Mode
	SET(ENABLE_NORMAL_SPI_MODE OFF)
	SET(ENABLE_DSPI_STREAM_MODE OFF)
	#Compile on/off SI551x support
	SET(ENABLE_SI551x OFF)
	#Compile on/off SI551x CLI Command support
	SET(ENABLE_SI551x_CLI OFF)
endif()

if(LA12XX_LA1238RDB)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DFECA_ENABLED")
	#Compile on/off DSPI Normal Mode
	SET(ENABLE_NORMAL_SPI_MODE OFF)
	SET(ENABLE_DSPI_STREAM_MODE OFF)
	#Compile on/off SI551x support
	SET(ENABLE_SI551x OFF)
	#Compile on/off SI551x CLI Command support
	SET(ENABLE_SI551x_CLI OFF)
endif()

if(LA12XX_LA1238CPE)
	#Compile on/off DSPI Normal Mode
	SET(ENABLE_NORMAL_SPI_MODE ON)
	SET(ENABLE_DSPI_STREAM_MODE OFF)
	#Compile on/off SI551x support
	SET(ENABLE_SI551x ON)
	#Compile on/off SI551x CLI Command support
	SET(ENABLE_SI551x_CLI ON)
endif()

if(LA12XX_LA1224CPE)
        #Compile on/off DSPI Normal Mode
        SET(ENABLE_NORMAL_SPI_MODE ON)
	SET(ENABLE_DSPI_STREAM_MODE OFF)
        #Compile on/off SI551x support
        SET(ENABLE_SI551x ON)
        #Compile on/off SI551x CLI Command support
        SET(ENABLE_SI551x_CLI ON)
endif()

#SI551x need DSPI
if(ENABLE_SI551x)
        SET(DSPI_ENABLED ON)
endif()

if(LA12XX_DRIVER_AGAVE_RF)
#Define e200 core number for agave driver
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DAGAVE -DAGAVE_LOCAL_CORE=0")
        SET(DSPI_ENABLED ON)
elseif(LA12XX_DRIVER_PATRON_RF)
	SET(ENABLE_RF ON)
	SET(DSPI_ENABLED ON)
	add_definitions( -DYUCCA_RF )
	#add_definitions( -DYUCCA_LLCP_STUB )
	add_definitions( -DYUC_ALL_FWCMDS=0 )
elseif(LA12XX_DRIVER_DIORA_RF)
#	SET(ENABLE_RF ON)
	add_definitions( -DDIORA_RF )
	add_definitions( -DDIORA_REFAPP4 )
elseif(LA12XX_DRIVER_DIORA_RF_LIB)
#	SET(ENABLE_RF ON)
	add_definitions( -DDIORA_RF )
	add_definitions( -DDIORA_REFAPP4 )
	SET(GEUL_DIORA_LIB	libdiora.a)
	SET(GEUL_DIORA_LIB_PATH "${RFDriverPath}/lib")
elseif(LA12XX_DRIVER_ICEWINGS_RF)
	SET(ENABLE_RF ON)
	SET(ICW_CLI OFF)
	SET(DSPI_ENABLED ON)
	add_definitions( -DICEWINGS_RF )
elseif(LA12XX_DRIVER_ADI_RF)
	SET(DSPI_ENABLED ON)
	add_definitions( -DADI_RF )
endif()

if(ENABLE_RF)
    add_definitions( -DENABLE_RF )
    add_definitions( -DRF_DEBUG_HIFCTRL=0 )
    SET(ENABLE_APIPLAYER ON)
    IF(LA12XX_BUILD_VARIANT MATCHES debug)
        add_definitions( -DRF_DEBUG=1 )
	ENDIF()
endif()

if (ENABLE_APIPLAYER)
add_definitions( -DAPIPLAYER_ENABLED)
endif()

if(LA12XX_DRIVER_PCI)
	add_definitions( -DLA12XX_DRIVER_PCI )
endif()

if(LA12XX_DRIVER_PCI_LAT_FP)
  add_definitions( -DLA12XX_DRIVER_PCI_LAT_FP )
endif()

if(LA12XX_DRIVER_DCS_LS)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DDCS_LS_ENABLED -DRF_LOCAL_CORE=0")
endif()

if(ENABLE_NORMAL_SPI_MODE OR ENABLE_SI551x OR ENABLE_SI551x_CLI)
    add_definitions( -DENABLE_NORMAL_SPI_MODE )
endif()

if(ENABLE_DSPI_STREAM_MODE)
    add_definitions( -DENABLE_DSPI_STREAM_MODE=1 )
endif()

if(ENABLE_SI551x OR ENABLE_SI551x_CLI)
	add_definitions( -DENABLE_SI551x )
endif()

if(ENABLE_SI551x_CLI)
	add_definitions( -DENABLE_SI551x_CLI )
endif()

# BBDEV or normal IPC
if(LA12XX_DRIVER_BBDEV_IPC)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DBBDEV_IPC_MODE")
	if(LA12XX_DRIVER_BBDEV_IPC_DESIGN2)
		SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DBBDEV_IPC_MODE_DESIGN2")
		# Enabling VSPA by default
		add_definitions( -DVSPA_ENABLE )
		# Enable auto run
		add_definitions( -DBBDEV_DEFAULT_FUNC_CALL )
	endif()
	add_definitions( -DMAX_LDPC_ENC_FECA_QUEUES=8 )
	add_definitions( -DMAX_LDPC_DEC_FECA_QUEUES=8 )
	add_definitions( -DMAX_POLAR_ENC_FECA_QUEUES=4 )
	add_definitions( -DMAX_POLAR_DEC_FECA_QUEUES=4 )
	add_definitions( -DMAX_RAW_QUEUES=8 )
	add_definitions( -DMAX_CHANNEL_DEPTH=16 )
endif()

if(LA12XX_FEATURE_L1C_REFAPP)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DL1C_REFAPP_ENABLE")
endif()

if(LA12XX_FEATURE_RUDEMO)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DL1C_RUDEMO_ENABLE")
endif()

# warm-up enable
if(LA12XX_FEATURE_WARMUP)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DWARMUP_ENABLE")
endif()

if(LA12XX_FEATURE_TESTFRAMEWORK)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DTESTFRAMEWORK_ENABLE")
IF(LA12XX_BUILD_VARIANT MATCHES debug)
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DCONFIG_TTI")
ENDIF()
endif()

if(LA12XX_FEATURE_HOST_CLI)
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DHOST_CLI_ENABLE")
endif()

# SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DBBDEV_DEFAULT_FUNC_CALL")
# SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DBBDEV_USE_SE_QDMA")

# LA12xx Errata
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DNXP_ERRATUM_A010650")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DNXP_ERRATUM_A050426")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DNXP_ERRATUM_A050272")
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DNXP_ERRATUM_A004737")

# CXX MACRO

# INCLUDE_DIRECTORIES
    INCLUDE_DIRECTORIES(${TOOLCHAIN_INC_DIR}/)
    INCLUDE_DIRECTORIES(${TOOLCHAIN_LIB_DIR}/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../FreeRTOS/portable/GCC/PowerPC_MPIC/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../FreeRTOS/include)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../include)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/avi)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/gpio)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/pmux)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/axiq)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/tbgen)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/watchdog)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/i2c)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/vcxo)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/tmu)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/pcal)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../lib/spinlocks/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../lib/ecc/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../lib/time/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../soc/include/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../soc/la12xx/include/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../boards/geul/include/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../arch/powerpc/e200/include/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../utils/)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../utils/FreeRTOS-Plus-CLI-Source/)
    INCLUDE_DIRECTORIES($ENV{COMMON_HEADERS_DIR}/)
    #INCLUDE_DIRECTORIES($ENV{LA12XX_COMMON_HDR_DIR}/)

if(LA12XX_FEATURE_WARMUP)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/lava)
endif()

if(LA12XX_FEATURE_TESTFRAMEWORK)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../utils/Test-Framework/)
endif()

if(ENABLE_SI551x)
	INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/si551x)
endif()

if(LA12XX_DRIVER_FECA)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/feca)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/feca/helpers/bbdev_feca)
endif()

if(LA12XX_FEATURE_L1C_REFAPP)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../utils/L1C-RefApp/)
endif()

if(LA12XX_FEATURE_RUDEMO)
   INCLUDE_DIRECTORIES(${ProjDirPath}/../../utils/L1C-RuDemo/)
endif()

if(LA12XX_DRIVER_BBDEV_IPC)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/bbdev_ipc)
else()
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/cpe_ipc)
endif()

if(ENABLE_RF)
	INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/rf/)
endif()

if(LA12XX_DRIVER_PCI OR LA12XX_DRIVER_PCI_LAT_FP)
	INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/pcie/)
endif()

if(LA12XX_DRIVER_AGAVE_RF)
	INCLUDE_DIRECTORIES(${RFDriverPath}/)
elseif(LA12XX_DRIVER_PATRON_RF)
	INCLUDE_DIRECTORIES(${RFDriverPath}/)
elseif(LA12XX_DRIVER_DIORA_RF)
	INCLUDE_DIRECTORIES(${RFDriverPath}/api/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/diora-freertos-cli/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/firmware/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/llcp/)
	INCLUDE_DIRECTORIES({RFDriverPath}/include/)
elseif(LA12XX_DRIVER_DIORA_RF_LIB)
	INCLUDE_DIRECTORIES(${RFDriverPath}/api/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/diora-freertos-cli/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/firmware/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/llcp/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/include/)
elseif(LA12XX_DRIVER_ICEWINGS_RF)
	INCLUDE_DIRECTORIES(${RFDriverPath}/)
elseif(LA12XX_DRIVER_ADI_RF)
	INCLUDE_DIRECTORIES(${RFDriverPath}/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/common/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/common/adi_error/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/common/adi_hal/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/devices/admv1128/include/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/devices/adf4368/include/)
	INCLUDE_DIRECTORIES(${RFDriverPath}/platforms/)
endif()

if(ENABLE_HAWK)
    INCLUDE_DIRECTORIES(${ProjDirPath}/../../drivers/hawk)
endif()

if(ENABLE_L1C)
	INCLUDE_DIRECTORIES(${ProjDirPath}/../../lib/l1c/)
endif()

if(ENABLE_APIPLAYER)
	INCLUDE_DIRECTORIES(${ProjDirPath}/../../lib/apiplayer/)
endif()

SET(GEUL_SOURCES
    "${ProjDirPath}/../../FreeRTOS/portable/GCC/PowerPC_MPIC/port.c"
    "${ProjDirPath}/../../FreeRTOS/portable/GCC/PowerPC_MPIC/portasm.S"
    "${ProjDirPath}/../../FreeRTOS/portable/MemMang/heap_4.c"
    "${ProjDirPath}/../../FreeRTOS/croutine.c"
    "${ProjDirPath}/../../FreeRTOS/event_groups.c"
    "${ProjDirPath}/../../FreeRTOS/list.c"
    "${ProjDirPath}/../../FreeRTOS/queue.c"
    "${ProjDirPath}/../../FreeRTOS/tasks.c"
    "${ProjDirPath}/../../FreeRTOS/timers.c"
    "${ProjDirPath}/../../drivers/serial/debug_console.c"
    "${ProjDirPath}/../../drivers/serial/print_scan.c"
    "${ProjDirPath}/../../drivers/serial/serial_ns16550.c"
    "${ProjDirPath}/../../drivers/serial/mem_log.c"
    "${ProjDirPath}/../../drivers/mpic/mpic.c"
    "${ProjDirPath}/../../drivers/mpic/interrupt_event_handler.c"
    "${ProjDirPath}/../../drivers/qdma/nxp-qdma.c"
    "${ProjDirPath}/../../drivers/qdma/nxp-qdma-legacy.c"
    "${ProjDirPath}/../../drivers/watchdog/watchdog.c"
    "${ProjDirPath}/../../arch/powerpc/e200/start.S"
    "${ProjDirPath}/../../arch/powerpc/e200/exceptions.S"
    "${ProjDirPath}/../../arch/powerpc/e200/interrupt.c"
    "${ProjDirPath}/../../arch/powerpc/e200/mpu.c"
    "${ProjDirPath}/../../soc/la12xx/soc.c"
    "${ProjDirPath}/../../boards/geul/board.c"
    "${ProjDirPath}/main.c"
    "${ProjDirPath}/../../drivers/gpio/gpio.c"
    "${ProjDirPath}/../../drivers/pmux/pmux.c"
    "${ProjDirPath}/../../lib/ipi/ipiQueue.c"
    "${ProjDirPath}/../../lib/spinlocks/spinlock.c"
    "${ProjDirPath}/../../lib/spinlocks/spinlock_arch.c"
    "${ProjDirPath}/../../lib/ecc/ecc.c"
    "${ProjDirPath}/../../lib/time/Time.c"
    "${ProjDirPath}/../../drivers/i2c/i2c.c"
    "${ProjDirPath}/../../drivers/tmu/tmu.c"
   )

if (LA12XX_LA1224)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/pcal/pcal.c"
    )
endif()

if(LA12XX_MTD_I2C_DIODE_TEMP)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/tmu/tmu_i2c.c"
    )
endif()

if(LA12XX_MTD_I2C_PWR_INFO)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/ina220/ina220.c"
    )
endif()

if(LA12XX_FEATURE_WARMUP)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/lava/lava_main.c"
    )
endif()

if(LA12XX_DRIVER_DCS_LS)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/dcs/ls_dcs.c"
    "${ProjDirPath}/../../drivers/dcs/ls_dcs_b0.c"
    "${ProjDirPath}/../../drivers/dcs/dcs.c"
  )
endif()

if(LA12XX_DRIVER_AXIQ)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/axiq/axiq.c"
    )
endif()

if(DSPI_ENABLED)
SET(GEUL_SOURCES ${GEUL_SOURCES}
	"${ProjDirPath}/../../drivers/dspi/fsl_dspi.c"
	)
endif()

IF(LA12XX_DRIVER_PCI OR LA12XX_DRIVER_PCI_LAT_FP)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/pcie/nxp-pcie.c"
    "${ProjDirPath}/../../drivers/pcie/nxp-pcie-rc.c"
    "${ProjDirPath}/../../drivers/pcie/nxp-pcie-common.c"
    "${ProjDirPath}/../../drivers/pcie/nxp-pcie-ep-firmware.c"
    "${ProjDirPath}/../../drivers/pcie/nxp-pcie-ep-host-driver.c"
    "${ProjDirPath}/../../drivers/pcie/nxp-pcie-ep-host-driver-hook.c"
    )
endif()

if(ENABLE_SI551x)
	SET(GEUL_SOURCES ${GEUL_SOURCES}
		"${ProjDirPath}/../../drivers/si551x/sync_timing_device.c"
		"${ProjDirPath}/../../drivers/si551x/sync_timing_device_cli.c"
		)
endif()

if(LA12XX_DRIVER_BBDEV_IPC)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/bbdev_ipc/bbdev_ipc.c"
  )
else ()
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/cpe_ipc/ipc.c"
  )
endif()

if(LA12XX_LA1224 OR LA12XX_LA1246)
	SET(GEUL_SOURCES ${GEUL_SOURCES}
		"${ProjDirPath}/../../boards/geul/la1224.c"
		)
endif()
if(LA12XX_LA1238RDB OR LA12XX_LA1238CPE)
	SET(GEUL_SOURCES ${GEUL_SOURCES}
		"${ProjDirPath}/../../boards/geul/la1238rdb.c"
		)
endif()

if(LA12XX_DRIVER_TBGEN)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/tbgen/la12xx_tbgen.c"
  )
SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}  -DTBGEN_ENABLED")
endif()

if(LA12XX_DRIVER_FLEXSPI)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/flexspi/fspi.c"
)
if(LA12XX_FEATURE_TESTFRAMEWORK)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../utils/Test-Framework/tc_xspi.c"
   )
endif()
endif()

IF(LA12XX_FEATURE_L1C_REFAPP OR LA12XX_FEATURE_HOST_CLI OR LA12XX_FEATURE_RUDEMO)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../utils/FreeRTOS-Plus-CLI-Source/FreeRTOS_CLI.c"
    "${ProjDirPath}/../../utils/command_from_shell.c"
    "${ProjDirPath}/../../utils/UARTCommandConsole.c"
   )
ENDIF()

if(LA12XX_FEATURE_TESTFRAMEWORK)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../utils/Test-Framework/test_framework.c"
   ) 
  
if(LA12XX_DRIVER_BBDEV_IPC)
	if(LA12XX_DRIVER_BBDEV_IPC_DESIGN2)
      SET(GEUL_SOURCES ${GEUL_SOURCES}
	      "${ProjDirPath}/../../utils/Test-Framework/tc_bbdev_ipc_du.c"
	      "${ProjDirPath}/../../utils/Test-Framework/tc_bbdev_ipc_raw.c"
      )
   else()
      SET(GEUL_SOURCES ${GEUL_SOURCES}
	      "${ProjDirPath}/../../utils/Test-Framework/tc_bbdev_ipc.c"
	      "${ProjDirPath}/../../utils/Test-Framework/tc_bbdev_ipc_raw.c"
      )
   endif()
else()
   SET(GEUL_SOURCES ${GEUL_SOURCES}
       "${ProjDirPath}/../../utils/Test-Framework/tc_cpe_ipc.c"
	  )
   endif()	
endif()

if(LA12XX_DRIVER_FECA)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/feca/feca_main_lib.c"
    "${ProjDirPath}/../../drivers/feca/feca_lib_sl.c"
    "${ProjDirPath}/../../drivers/feca/feca_freertos_stubs.c"
   )

IF(LA12XX_BUILD_VARIANT MATCHES debug)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/feca/test/geul_feca_test.c"
    "${ProjDirPath}/../../drivers/feca/test/test_vectors.c"
   )
ENDIF()
endif()

if(LA12XX_FEATURE_L1C_REFAPP)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_cli.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_demo.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_vspa_agent.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_vspa_proc.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_axiq.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_time_utils.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_rf_ctrl.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_fwk_tasks.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_time_agent.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_time_proc.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_bench.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_dpd.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_debug.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_ipi.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_freq.c"
   )
endif()

if(LA12XX_FEATURE_RUDEMO)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_cli.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_demo.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_vspa_agent.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_vspa_proc.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_axiq.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_time_utils.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_rf_ctrl.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_fwk_tasks.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_time_agent.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_time_proc.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_bench.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_dpd.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_debug.c"
    "${ProjDirPath}/../../utils/L1C-RuDemo/l1c_ipi.c"
    "${ProjDirPath}/../../utils/L1C-RefApp/l1c_freq.c"
   )

    if(LA12XX_LA1238RDB)
	SET(GEUL_SOURCES ${GEUL_SOURCES}
		"${ProjDirPath}/../../utils/L1C-RuDemo/l1c_rf_ctrl_la1238rdb.c"
    )
    elseif(LA12XX_LA1224)
	SET(GEUL_SOURCES ${GEUL_SOURCES}
		"${ProjDirPath}/../../utils/L1C-RuDemo/l1c_rf_ctrl_la1224rdb.c"
    )
    endif()
endif()

SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/avi/geul_avi.c"
  )

if(ENABLE_RF)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/rf/rf_dev.c"
    "${ProjDirPath}/../../drivers/rf/rf_remote_isr.c"
    "${ProjDirPath}/../../drivers/rf/rf_core.c"
    "${ProjDirPath}/../../drivers/rf/rf_api.c"
    "${ProjDirPath}/../../drivers/rf/rf_remote.c"
    "${ProjDirPath}/../../drivers/rf/rf_local.c"
	)
endif()

if(ENABLE_APIPLAYER)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../lib/apiplayer/apiplayer.c"
    "${ProjDirPath}/../../drivers/rf/rfapiplayer.c"
	)
endif()

if(LA12XX_LA1224 AND ENABLE_RF)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/vcxo/vcxo.c"
    )
endif()

if(LA12XX_DRIVER_AGAVE_RF)
SET(GEUL_SOURCES ${GEUL_SOURCES}
	"${RFDriverPath}/agave_init.c"
	"${RFDriverPath}/agave_synth.c"
	"${RFDriverPath}/agave_demod.c"
	"${RFDriverPath}/agave_cli.c"
	)
elseif(LA12XX_DRIVER_PATRON_RF)
SET(GEUL_SOURCES ${GEUL_SOURCES}
	"${RFDriverPath}/yuc_rfic_cmd.c"
	"${RFDriverPath}/yuc_rfic.c"
	"${RFDriverPath}/yuc_init.c"
	"${RFDriverPath}/patron_fem.c"
	)
elseif(LA12XX_DRIVER_DIORA_RF)
SET(GEUL_SOURCES ${GEUL_SOURCES}
	"${RFDriverPath}/drv/mt3812_calib.c"
	"${RFDriverPath}/drv/mt3812_calib_common.c"
	"${RFDriverPath}/drv/mt3812_calib_tools.c"
	"${RFDriverPath}/drv/mt3812_calib_bw.c"
	"${RFDriverPath}/drv/mt3812_calib_iq.c"
	"${RFDriverPath}/drv/mt3812_calib_iq_v1.c"
	"${RFDriverPath}/drv/mt3812_calib_iq_v1_hs.c"
	"${RFDriverPath}/drv/mt3812_calib_lo.c"
	"${RFDriverPath}/drv/mt3812_calib_rxdc.c"
	"${RFDriverPath}/drv/mt3812_calib_rxdc_hs.c"
	"${RFDriverPath}/drv/mt3812_cmdapi.c"
	"${RFDriverPath}/drv/mt3812_drv.c"
	"${RFDriverPath}/port/diora_osal_freertos_la1224.c"
	"${RFDriverPath}/port/diora_phal_la1224_llcp.c"
	"${RFDriverPath}/port/diora_l1al_la1224.c"
	"${RFDriverPath}/diora-freertos-cli/src/diora-freertos-cli.c"
	"${RFDriverPath}/firmware/fw_init_mem_R2_7_001.c"
	"${RFDriverPath}/firmware/fw_prog_mem_R2_7_001.c"
	"${RFDriverPath}/llcp/llcp_la12xx.c"
	)
elseif(LA12XX_DRIVER_DIORA_RF_LIB)
SET(GEUL_SOURCES ${GEUL_SOURCES}
	"${RFDriverPath}/port/diora_osal_freertos_la1224.c"
	"${RFDriverPath}/port/diora_phal_la1224_llcp.c"
	"${RFDriverPath}/port/diora_l1al_la1224.c"
	"${RFDriverPath}/diora-freertos-cli/src/diora-freertos-cli.c"
	"${RFDriverPath}/firmware/fw_init_mem_R2_7_001.c"
	"${RFDriverPath}/firmware/fw_prog_mem_R2_7_001.c"
	"${RFDriverPath}/llcp/llcp_la12xx.c"
	)
elseif(LA12XX_DRIVER_ICEWINGS_RF)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${RFDriverPath}/icewings_init.c"
    "${RFDriverPath}/icewings_synth.c"
    "${RFDriverPath}/icewings_cli.c"
    "${RFDriverPath}/icw_cmd.c"
    "${RFDriverPath}/icw_dspi.c"
    "${RFDriverPath}/icw_gpio.c"
	)

	#if(ICW_CLI)
	#add_definitions( -DICW_CLI )
	#SET(GEUL_SOURCES ${GEUL_SOURCES}
	#    "${RFDriverPath}/icewings_cli.c"
	#)
	#endif()
elseif(LA12XX_DRIVER_ADI_RF)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${RFDriverPath}/aib.c"
    "${RFDriverPath}/aib_cli.c"
    "${RFDriverPath}/admv48281.c"
    "${RFDriverPath}/admv1128.c"
    "${RFDriverPath}/admv1128_cli.c"
    "${RFDriverPath}/adf4368.c"
    "${RFDriverPath}/adf4368_cli.c"
    "${RFDriverPath}/platforms/adi_platform.c"
    "${RFDriverPath}/devices/admv1128/src/adi_admv1128_hal.c"
    "${RFDriverPath}/devices/admv1128/src/adi_admv1128.c"
    "${RFDriverPath}/devices/adf4368/src/adi_adf4368_hal.c"
    "${RFDriverPath}/devices/adf4368/src/adi_adf4368.c"
    )
endif()
if(ENABLE_HAWK)
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../drivers/hawk/hawk.c"
    )
endif()


if(VSPA_OV_PEBM_RESERVE)
    add_definitions( -DVSPA_OV_PEBM_ENABLED=1 )
else ()
IF(LA12XX_BUILD_VARIANT MATCHES debug)
IF(LA12XX_FEATURE_TESTFRAMEWORK)
#overlay are for data usages
SET(GEUL_SOURCES ${GEUL_SOURCES}
    "${ProjDirPath}/../../utils/Test-Framework/tc_overlay_data.c"
    )
ENDIF()
ENDIF()
endif()

IF(LA12XX_BUILD_VARIANT MATCHES debug)
IF(LA12XX_FEATURE_TESTFRAMEWORK)
SET(GEUL_SOURCES ${GEUL_SOURCES}
	"${ProjDirPath}/../../utils/Test-Framework/tc_axiq.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_msi.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_timer.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_vspa.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_avi.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_gpio.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_exception.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_la12xx_tbgen.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_ipi.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_qdma.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_qdma_legacy.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_spinlock.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_system.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_watchdog.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_memcheck.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_pcimsi.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_logger.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_i2c_eeprom.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_i2c_tmu.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_tmu.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_i2c_lmp92xx.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_ipisr.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_latency.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_smem_text.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_core_smem_text.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_cpu_loader.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_peb_port3.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_heap_usage.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_la12xx_float.c"
  )
if( NOT CPE_IPC)
SET(GEUL_SOURCES ${GEUL_SOURCES}
	"${ProjDirPath}/../../drivers/feca/helpers/bbdev_feca/shared_ch_hw_param_du.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_feca_bbdev.c"
	"${ProjDirPath}/../../utils/Test-Framework/tc_feca_bbdev_tv.c"
  )
endif()

if(ENABLE_DSPI_STREAM_MODE)
SET(GEUL_SOURCES ${GEUL_SOURCES}
        "${ProjDirPath}/../../utils/Test-Framework/tc_dspi.c"
  )
endif()


ENDIF()
ENDIF()

ADD_EXECUTABLE(geul ${GEUL_SOURCES})

SET_TARGET_PROPERTIES(geul PROPERTIES OUTPUT_NAME "geul_e200.elf")

TARGET_LINK_LIBRARIES(geul -Wl,--start-group)
# LIBRARIES
#IF(CMAKE_BUILD_TYPE MATCHES Debug)
#ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
#ENDIF()

if(LA12XX_DRIVER_DIORA_RF_LIB)
	TARGET_LINK_LIBRARIES(geul ${GEUL_DIORA_LIB_PATH}/${GEUL_DIORA_LIB})
endif()

# SYSTEM LIBRARIES
TARGET_LINK_LIBRARIES(geul m)
TARGET_LINK_LIBRARIES(geul c)
TARGET_LINK_LIBRARIES(geul gcc)
TARGET_LINK_LIBRARIES(geul nosys)
TARGET_LINK_LIBRARIES(geul -Wl,--end-group)
if(ENABLE_L1C)
	TARGET_LINK_LIBRARIES(geul ${GEUL_L1C_LIB_PATH}/${GEUL_L1C_LIB})
endif()

# MAP FILE
SET(CMAKE_EXE_LINKER_FLAGS_DEBUG   "${CMAKE_EXE_LINKER_FLAGS_DEBUG}   -Xlinker -Map=debug/geul_e200.map")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -Xlinker -Map=release/geul_e200.map")

# BIN AND HEX
ADD_CUSTOM_COMMAND(TARGET geul POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Oihex   ${EXECUTABLE_OUTPUT_PATH}/geul_e200.elf ${EXECUTABLE_OUTPUT_PATH}/geul_e200.hex)
ADD_CUSTOM_COMMAND(TARGET geul POST_BUILD COMMAND ${CMAKE_OBJCOPY} -Obinary ${EXECUTABLE_OUTPUT_PATH}/geul_e200.elf ${EXECUTABLE_OUTPUT_PATH}/geul_e200.bin)
if(LA12XX_FEATURE_RUDEMO)
ADD_CUSTOM_COMMAND(TARGET geul POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${EXECUTABLE_OUTPUT_PATH}/geul_e200.elf ${EXECUTABLE_OUTPUT_PATH}/geul_e200_rudemo.elf)
endif()

# SHA-ID AND TIMESTAMP
execute_process (
	COMMAND bash -c "echo -n `date +'%F %T'` : `git describe --abbrev=4 --tags --dirty`"
	OUTPUT_VARIABLE GITVERSION
	)

file(WRITE "sha-id.txt" "${GITVERSION}")
add_definitions( -DREL_VERSION="${GITVERSION}" )
