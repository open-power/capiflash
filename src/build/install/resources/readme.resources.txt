The resources directory contains various external tools and applications needed by the IBM Data Engine for NoSQL.

80-cxl.rules
Purpose: udev rules for /dev/cxl/afuX.0s and X.0m that enable members of the cxl group to access the slave device(s)

80-cxlflash.rules
Purpose: udev rules for /dev/sdX and sgX that call enable user space to notify the cxlflash module about device status

blacklist-cxlflash.conf
Purpose: temporarily blacklist the cxlflash driver by placing this in /etc/modprobe.d . this is used by reload_all_adapters (see below).

capi_flash.pl
Purpose: flash AFU factory or user images.

corsa_surelock.xxx.bin
Purpose: Accelerator binary image

cxlffdc
Purpose: Gather PSL and AFU debug data, and place into a tarball for HW diagnosis

flash_all_adapters
Purpose: Wrapper script for capi_flash.pl, used by debian package post-installer


reload_all_adapters
Purpose: Simple Wrapper to enable perst / reset of the adapters after a flash update without a reboot

flash_factory_image
Purpose: Wrapper script for capi_flash.pl, used by CSC manufacturing to write factory image prior to shipmnet

license/*
Purpose: License terms / conditions for IBM Data Engine for NoSQL User Libraries

postinstall
Purpose: initial setup of system - used by clients or ssrs

psl_trace_dump
Purpose: collected PSL FFDC / traces on a failed system


capikvutils.sh
Purpose: Utility code for cxlflash and ibmcapikv tooling

cxlfrefreshluns cxlfsetlunmode cxlfstatus
Purpose: suite of utilities for manipulating the cxlflash driver
