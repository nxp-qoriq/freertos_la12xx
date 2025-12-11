// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2023 NXP
 *
 */

PCIe drivers adds support to communicates with directly connected PCIe devices.
PCIe driver support is devided in three parts.
- RC Side: PCIe RC-FW firmware driver
- RC Side: PCIe EP-Host driver
- EP Side: PCIe EP-FW firmware driver


1 - PCIe RC-FW Firmware Driver:
-------------------------------
PCIe RC-FW driver is needed to discover, identify and configure connected PCIe EP devices.

2 - PCIe EP-FW Firmware Driver:
-------------------------------
PCIe EP firmware driver is needed to map it’s BAR address to its’s internal memory map so that
host side EP driver can access PCIe EP memory.

3- PCIe EP Host Driver:
-----------------------
PCIe EP host driver should be able to use APIs provided by PCIe RC firmware driver to get desired
information and access connected EP devices.

Code for PCIe RC-FW, PCIe EP-FW and PCIe EP Host Driver is divided into below files.

nxp-pcie.c                          : Contains top level API to call RC or EP firmware driver based
                                      on configuration.

nxp-pcie-common.c                   : Contains common code to be used by PCIe RC/EP framwork

nxp-pcie-rc.c                       : Contains PCIe-RC firmware driver code for LA12xx PCIe
                                      conroller. It detects, enumurates and assign resources to
									  connected EP device.

nxp-pcie-ep-firmware.c              : Contains PCIe-EP firmware driver code for LA12xx PCIe
                                      conroller. For any other PCIe controller refer this file to
									  develop code for underneath EP device.

nxp-pcie-ep-host-driver.c           : Contains code for PCIe EP host driver for NXP PCIe controller.
                                      Refer this file to develop code for Non-NXP underneath EP
									  device.

nxp-pcie-ep-host-driver-hook.c      : Contains code to test and execute PCIe EP host driver based on
                                      VendorID/DeviceID value.
