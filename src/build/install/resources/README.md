# Utilities which work for both EJ16(Corsa) and EJ1K(FlashGT)

### query and validate the current software versions
```
> /opt/ibm/capikv/bin/cflash_version
INFO:      OS:            16.04.2
INFO:      cxlflash:      4.1-2359-bb8a68d
INFO:      Kernel:        4.4.0-72.93
INFO:      0000:01:00.0   10140601 161114N4
INFO:      0004:01:00.0   10140601 161114N4
INFO:      0040:01:00.0   101404cf 160512D1
```
### query cards, luns, lun mode
```
> /opt/ibm/capikv/bin/cxlfstatus

CXL Flash Device Status

Found 0601 0000:01:00.0 U78CB.001.WZS0073-P1-C7
  Device:  SCSI       Block    Mode       LUN WWID                          Persist
  sg16:    3:0:0:0,      ,     superpipe, 60025380025382462300034000000000, sg0700,
  sg17:    3:1:0:0,      ,     superpipe, 60025380025382463300053000000000, sg0710,

Found 0601 0004:01:00.0 U78CB.001.WZS0073-P1-C6
  Device:  SCSI       Block    Mode       LUN WWID                          Persist
  sg8:     1:0:0:0,      ,     superpipe, 60025380025382462300058000000000, sg0600,
  sg9:     1:1:0:0,      ,     superpipe, 60025380025382462300066000000000, sg0610,

Found 04cf 0040:01:00.0 U78CB.001.WZS0073-P1-C5
  Device:  SCSI       Block    Mode       LUN WWID                          Persist
  sg11:    2:0:0:1,   sde,     legacy,    60050768218b0818200000000f00050a, sg0501,    sd0501
  sg12:    2:0:0:2,   sdf,     legacy,    60050768218b0818200000001000050b, sg0502,    sd0502
  sg14:    2:1:0:1,   sdg,     legacy,    60050768218b0818200000000f00050a, sg0511,    sd0511
  sg15:    2:1:0:2,   sdh,     legacy,    60050768218b0818200000001000050b, sg0512,    sd0512
```
### change the state of a lun (superpipe/legacy)
```
> /opt/ibm/capikv/bin/cxlfsetlunmode 60050768218b081820000000010004e2 1
INFO: Adding LUN 60050768218b081820000000010004e2 to Super IO table.
SUCCESS

> /opt/ibm/capikv/bin/cxlfsetlunmode 60050768218b081820000000010004e2 0
INFO: Removing LUN 60050768218b081820000000010004e2 from Super IO table.
SUCCESS
```
### refresh and query the cards, luns, lun mode; and create persistent links
```
> /opt/ibm/capikv/bin/cxlfrefreshluns
Refer to /tmp/cxlflog.root.log for detailed table update logs.
CXL Flash Device Status

Found 0601 0000:01:00.0 U78CB.001.WZS0073-P1-C7
  Device:  SCSI       Block    Mode       LUN WWID                          Persist
  sg16:    3:0:0:0,      ,     superpipe, 60025380025382462300034000000000, sg0700,
  sg17:    3:1:0:0,      ,     superpipe, 60025380025382463300053000000000, sg0710,

Found 0601 0004:01:00.0 U78CB.001.WZS0073-P1-C6
  Device:  SCSI       Block    Mode       LUN WWID                          Persist
  sg8:     1:0:0:0,      ,     superpipe, 60025380025382462300058000000000, sg0600,
  sg9:     1:1:0:0,      ,     superpipe, 60025380025382462300066000000000, sg0610,

Found 04cf 0040:01:00.0 U78CB.001.WZS0073-P1-C5
  Device:  SCSI       Block    Mode       LUN WWID                          Persist
  sg11:    2:0:0:1,   sde,     legacy,    60050768218b0818200000000f00050a, sg0501,    sd0501
  sg12:    2:0:0:2,   sdf,     legacy,    60050768218b0818200000001000050b, sg0502,    sd0502
  sg14:    2:1:0:1,   sdg,     legacy,    60050768218b0818200000000f00050a, sg0511,    sd0511
  sg15:    2:1:0:2,   sdh,     legacy,    60050768218b0818200000001000050b, sg0512,    sd0512
```
### query the unused persistent lun names
```
> /opt/ibm/capikv/bin/cflash_plinks -unused
```
### delete a single persistent lun name
```
> /opt/ibm/capikv/bin/cflash_plinks -r sd0501
```
### delete the unused persistent lun names (use to cleanup stale names)
```
> /opt/ibm/capikv/bin/cflash_plinks -rmunused
```
### delete all the persistent lun names
```
> /opt/ibm/capikv/bin/cflash_plinks -rmall
```
### query the devices for an adapter type
```
> /opt/ibm/capikv/afu/cflash_devices.pl -t 0601 -s
/dev/sg8:/dev/sg10:/dev/sg11:/dev/sg9

> /opt/ibm/capikv/afu/cflash_devices.pl -t 04cf -s
/dev/sg12:/dev/sg13
```
### query the capacity for an adapter type
```
> /opt/ibm/capikv/afu/cflash_capacity.pl -v
/dev/sg8:  894        NSID: 1 NSZE: 6fc81ab0  NCAP: 6fc81ab0  NUSE: 9c14fb0 percent used: 8.73
/dev/sg9:  894        NSID: 1 NSZE: 6fc81ab0  NCAP: 6fc81ab0  NUSE: 36b5470 percent used: 3.06
/dev/sg10: 894        NSID: 1 NSZE: 6fc81ab0  NCAP: 6fc81ab0  NUSE: 3a56950 percent used: 3.26
/dev/sg11: 894        NSID: 1 NSZE: 6fc81ab0  NCAP: 6fc81ab0  NUSE: 2063b58 percent used: 1.81
3576

> /opt/ibm/capikv/afu/cflash_capacity.pl -v -t 04cf
/dev/sg12: 1000    
1000
```
### query the AFU firmware level and card serial#
```
> /opt/ibm/capikv/afu/capi_flash.pl -l
Found CAPI device 10140601 afu0 0000:01:00.0 U78CB.001.WZS0073-P1-C7 07210026       =>160910N1
Found CAPI device 10140601 afu1 0002:01:00.0 U78CB.001.WZS0073-P1-C6 07210024       =>160910N1
Found CAPI device 101404cf afu2 0004:01:00.0 U78CB.001.WZS0073-P1-C5 YH10HT55F006   =>160512D1
Found CAPI device 101404cf afu3 0005:01:00.0 U78CB.001.WZS0073-P1-C3 0000017800823  =>160512D1
```
### run 100% random 4k read IO to exercise a vlun
```
> /opt/ibm/capikv/bin/blockio -d /dev/sg8
r:100 q:128 s:4 p:0 n:1 i:0 v:0 eto:1000000 miss:13/13528257 lat:518 mbps:955 iops:244680
```
### run write/read/compare IO using the Arkdb
```
> /opt/ibm/capikv/bin/kv_perf -d /dev/sg8 -M
ASYNC: /dev/sg8: QD:100
 tops:  380000 tios:  750000 op/s: 190000 io/s: 375000 secs:2
 tops: 1610000 tios: 1610000 op/s: 805000 io/s: 805000 secs:2
```
### run performance checks to get a quick evaluation of the health of an adapter/lun
##### (/dev/sgN must be in "superpipe" mode)
```
> /opt/ibm/capikv/afu/cflash_perfcheck.pl -d /dev/sg8
latency       vlun    plun
  rd            48      25
  wr            20      19
1P QD1
  rd         20025   37888
  wr         48210   50834
1P QD128
  rd        324157  322786
  wr        184825  186422
1P QD32 N16
  rd                322894
  wr                186724
```
# Utilities which work for EJ1K(FlashGT) only

### query the status of the NVMe ports
```
> /opt/ibm/capikv/afu/cxl_afu_status -d afu0.0m
 NVMe0: GEN3 width=4 link up
 NVMe1: GEN3 width=4 link up
```
### query the current iops and rd% of each lun, as a single line or a histogram
```
> /opt/ibm/capikv/afu/cflash_perf.pl -l -s 1
Tue Sep 13 10:26:53  =>   sg22:203313  sg23:197664  sg8:205370  sg9:208960    total:815308   rds:100%
Tue Sep 13 10:26:54  =>   sg22:222474  sg23:215126  sg8:222352  sg9:224629    total:884581   rds:100%
Tue Sep 13 10:26:55  =>   sg22:222855  sg23:214484  sg8:221969  sg9:223865    total:883173   rds:100%
```
### query the temperatures of the NVMe sticks
```
> /opt/ibm/capikv/afu/cflash_temp.pl

Found 0601 0000:01:00.0 afu0.0m
  FPGA Temperature is 52.49218750 degrees Celsius
  sg22:   Current temperature = 37 C
  sg23:   Current temperature = 37 C

Found 0601 0002:01:00.0 afu1.0m
  FPGA Temperature is 55.91015625 degrees Celsius
  sg8:   Current temperature = 35 C
  sg9:   Current temperature = 35 C
```
### query the wear of the NVMe sticks
```
> /opt/ibm/capikv/afu/cflash_wear.pl

Found 0601 0000:01:00.0 afu0.0m
  sg22:   Percentage used endurance indicator: 2%
  sg23:   Percentage used endurance indicator: 2%

Found 0601 0002:01:00.0 afu1.0m
  sg8:   Percentage used endurance indicator: 1%
  sg9:   Percentage used endurance indicator: 1%
```
### query the NVMe stick controller firmware level
```
> /opt/ibm/capikv/afu/cflash_stick.pl -l

Found 0601 0000:01:00.0 afu0.0m
 /dev/sg22
  NVMe0 Version = BXV7301Q
  NVMe0 NEXT    = BXV7301Q
  NVMe0 STATUS  = 0x701
 /dev/sg23
  NVMe1 Version = BXV7301Q
  NVMe1 NEXT    = BXV7301Q
  NVMe1 STATUS  = 0x701

Found 0601 0002:01:00.0 afu1.0m
 /dev/sg8
  NVMe0 Version = BXV7301Q
  NVMe0 NEXT    = BXV7301Q
  NVMe0 STATUS  = 0x701
 /dev/sg9
  NVMe1 Version = BXV7301Q
  NVMe1 NEXT    = BXV7301Q
  NVMe1 STATUS  = 0x701
```