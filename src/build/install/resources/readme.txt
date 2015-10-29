IBM Data Engine for NoSQL - Power System Edition

Please review the licenses available in /opt/ibm/capikv/license/

FlashSystem ssh key configuration no longer required for this solution. Mapping or unmapping LUNS may be performed via the FlashSystem GUI.

Please ensure that all fiber channel cables are connected point-to-point between the accelerator cards and the FlashSystem, and ensure that all LUNs (or vdisks) mapped to the accelerators are 4096-byte block formatted disks.

This software includes:
    include     Header files for application development
    lib         Libraries for exploitation of CAPI Flash APIs
    license     Licenses for the IBM Data Engine for NoSQL - Power Systems Edition
    examples    Example binaries and source code for application development

Common Tasks
============

Host Creation in the FlashSystem
--------------------------------
To determine the CAPI Flash World Wide Port Names (WWPN) for each of the present accelerator's ports (to allow one to provision them in FlashSystem to specific ports), one must use the lspci command. First, use it without arguments as "lspci |grep 'IBM Device 04cf'", which will output similar to the following:
    0000:01:00.0 Processing accelerators: IBM Device 04cf (rev 01)
    0002:01:00.0 Processing accelerators: IBM Device 04cf (rev 01)

From the above list, select an adapter using the identifiers in the left column. Thus to get WWPN for the first adapter listed one would use the command "lspci -s 0000:01:00.0 -vv |grep -e V5 -e V6", which will show output similar to:
    lspci -s 0000:01:00.0 -vv |grep -e V5 -e V6
    [V5] Vendor specific: 5005076069800230
    [V6] Vendor specific: 5005076069800231
Thus the WWPNs of this adapter are: 5005076069800230, and 5005076069800231. Use these WWPNs to create a new host (or hosts) in the FlashSystem GUI or CLI.


Controlling Access to the Accelerator
-------------------------------------

By default, installation of this package creates a "cxl" user and group, and adds udev rules to restrict read / write to the accelerator's volumes to members of the "cxl" system group. To enable an account to read/write the accelerator, add the account to the cxl group. For example:
    sudo usermod -a -G cxl userid



Viewing the status of the accelerator
-------------------------------------

This package includes convenience scripts to display the status of the volumes mapped to the accelerator. To view the status of each adapter's LUN, use:
    sudo /opt/ibm/capikv/bin/cxlfstatus

Volumes may be in either "legacy" or "superpipe" mode. Volumes will default to legacy mode. Volumes must be in 'superpipe' mode for exploitation by the CAPI Flash block or arkdb APIs.

The below example shows two accelerators, each with two ports, an a single volume mapped to each port's WWPN. One volume is in "legacy" mode, and three volumes are in "superpipe" mode. The WWID for each volume in the FlashSystem are displayed for convenience of administration. This matches the WWID shown in the FlashSystem GUI or CLI.

ibm@power8:~$ sudo /opt/ibm/capikv/bin/cxlfstatus
CXL Flash Device Status
Device:       SCSI  Block       Mode                          LUN WWID
   sg9:   33:0:0:0,   sdc,    legacy, 60050768218b0818200000000400006e
  sg10:   33:1:0:0,   sdd, superpipe, 60050768218b0818200000000600006f
  sg11:   34:0:0:0,   sde, superpipe, 60050768218b08182000000007000070
  sg12:   34:1:0:0,   sdf, superpipe, 60050768218b0818200000000300006d


Setting the mode for a volume
-----------------------------

As shown in the example above, each volume may be in either "legacy" or "superpipe" mode. To set the mode for a volume, use the following command:
    sudo /opt/ibm/capikv/bin/cxlfsetlunmode <LUN> <Mode>

For example, LUN modes may be "0" for legacy, or "1" for superpipe. As an example:
    ibm@power8:~$ /opt/ibm/capikv/bin/cxlfsetlunmode 60050768218b0818200000000400006e 1
    INFO: Adding LUN 60050768218b0818200000000400006e to Super IO table.
    SUCCESS

After a LUN is set to superpipe mode, all paths to that volume will also be set to superpipe.
