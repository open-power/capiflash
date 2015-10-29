#!/bin/bash -e
#  IBM_PROLOG_BEGIN_TAG
#  This is an automatically generated prolog.
#
#  $Source: src/build/install/resources/capikvutils.sh $
#
# IBM Data Engine for NoSQL - Power Systems Edition User Library Project
#
# Contributors Listed Below - COPYRIGHT 2014,2015
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.
#
# IBM_PROLOG_END_TAG

CAPIKVROOT=/opt/ibm/capikv
SIOTABLE=$CAPIKVROOT/etc/sioluntable.ini
LOGFILE=/tmp/cxlflog.${USER}.log

#cxlflash LUN modes
LUNMODE_LEGACY=0;
LUNMODE_SIO=1;

#common return codes
ENOENT=2;
EIO=5;
EACCES=13;
EINVAL=22;


die()
{
    echo "$1";
    exit 1;
}

#@desc manipulate the SIO LUN table, adding or deleting an entry
#@param $1 - LUN
#@param $2 - mode
#@returns rc to 0 on success, OR sets return code to non-zero value on error
chluntable()
{
    local targetlun=$1;
    local targetmode=$2;
    local currmode="unknown";
    getluntablestate  currmode $targetlun;
    case "$currmode-$targetmode" in
        "$LUNMODE_LEGACY-$LUNMODE_LEGACY" )
            echo "INFO: LUN $targetlun is already in legacy mode. No action needed.";
        ;;
        "$LUNMODE_LEGACY-$LUNMODE_SIO" )
            echo "INFO: Adding LUN $targetlun to Super IO table.";
            echo "$targetlun=1" >> $SIOTABLE;
        ;;
        "$LUNMODE_SIO-$LUNMODE_LEGACY" )
            echo "INFO: Removing LUN $targetlun from Super IO table.";
            sed "/$targetlun=.*/d" $SIOTABLE > ${SIOTABLE}.new;
            mv $SIOTABLE ${SIOTABLE}.bak;
            mv ${SIOTABLE}.new $SIOTABLE;
        ;;
        "$LUNMODE_SIO-$LUNMODE_SIO" )
            echo "INFO: LUN $targetlun is already in SIO mode. No action needed.";
        ;;
        * )
            echo "LUN $targetlun has unknown LUN Status ($currmode) or target mode ($targetmode)."
            return $EINVAL;
        ;;
    esac
   #check if the LUN's there, and if we're deleting, delete it. If its not and adding, add it.
   #exclusive lock  
}    

#@desc map a block device to its underlying sg device
#@param $1 - output variable
#@param $2 - block device (e.g. "sdc")
#@returns sg device string (e.g. "sg4") and sets rc to 0 on success, OR sets return code to non-zero value on error
getsgfromblock()
{
    local __resultvar=$1;
    local blockdev=$2;
    local sgdev="unknown";
    if [[ -e /sys/class/block/$blockdev/device/scsi_generic ]]; then
        sgdev=`ls /sys/class/block/$blockdev/device/scsi_generic`;
    else
        echo "Invalid sg device: $dev"
        return $ENOENT;
    fi
    eval $__resultvar="'$sgdev'"
}    

#@desc get the 32-character hex LUN identifier that backs a given sg device
#@param $1 - output variable
#@param $2 - sg device (e.g. "sg12")
#@returns hex string LUN ID and sets rc to 0 on success, OR sets return code to non-zero value on error
getlunid()
{
    local __resultvar=$1;
    local lun=`/lib/udev/scsi_id -d /dev/$2 --page=0x83 --whitelisted`;
    lun=`echo $lun | awk '{print substr($1,2); }'`; #cut off the first digit ONLY, since we want the LUN ID, not the inquiry response which leads with a nibble indicating some scsi data we don't care about.
    local lunlength=${#lun};
    if [[ "$lunlength" -eq "0" ]]; then
        lun="unknown";
        return $ENOENT;
    fi
    eval $__resultvar="'$lun'"
}

#@desc get the cxlflash device mode for the given sg device
#@param $1 - output variable
#@param $2 - sg device (e.g. "sg12")
#@returns lun mode integer and sets rc to 0 on success, OR sets return code to non-zero value on error
getmode()
{
    local __resultvar=$1;
    local mode="unknown";
    if [[ -e /sys/class/scsi_generic/$2/device/mode ]]; then
        mode=`cat /sys/class/scsi_generic/$2/device/mode`;
    else
        return $ENOENT;
    fi
    eval $__resultvar="'$mode'"
}

#@desc get the block device for the given sg device
#@param $1 - output variable
#@param $2 - sg device (e.g. "sg12")
#@returns block device (e.g. "sdc" and sets rc to 0 on success, OR sets return code to non-zero value on error
getblockdev()
{
    local __resultvar=$1;
    local dev=$2;
    local block="unknown";
    if [[ -e /sys/class/scsi_generic/$dev/device/block ]]; then
        local block=`ls /sys/class/scsi_generic/$dev/device/block`;
    else
        #echo "Invalid block device: $dev"
        #this will naturally occur if we are in superpipe mode, or if the LUN mappings are discontinuous (e.g. LUN1 exists, but LUN0 does not).
        return $ENOENT;
    fi
    eval $__resultvar="'$block'"
}


#@desc get the scsi topology for a given sg device (e.g. a:b:c:d)
#@param $1 - output variable
#@param $2 - sg device (e.g. "sg12")
#@returns scsi topology string, colon separated, e.g. '1:2:3:4' and sets rc to 0 on success, OR sets return code to non-zero value on error
getscsitopo()
{
    local __resultvar=$1;
    local dev=$2;
    local pathname=`ls -d /sys/class/scsi_generic/$dev/device/scsi_device/*`;
    local rslt=`basename $pathname`;
    eval $__resultvar="'$rslt'"
}

ctrlblockdevmap()
{
    local dev=$1;
    local scsitopo=0;
    local tgmode=$2;
    local scsiaction="noop";
    local blockdev="unknown";
    getblockdev blockdev $dev || blockdev="unknown";
    getscsitopo scsitopo $dev;
    #only unmap the device if the block device is NOT unknown, and we want to go to SIO mode
    if [[ ( "$tgmode" == "$LUNMODE_SIO" ) &&  ( "$blockdev" != "unknown" ) ]]; then
        scsiaction="unbind";
    #only map the device if the block device is unknown, and want to go to LEGACY mode
    elif [[ ("$tgmode" == "$LUNMODE_LEGACY") &&  ("$blockdev" == "unknown") ]]; then
        scsiaction="bind";
    fi
    #if there's something to do, call either bind or unbind appropriately
    if [ "$scsiaction" != "noop" ]; then
        echo "INFO: ${scsiaction}ing $dev's block device, currently $blockdev" >> $LOGFILE;
        echo -n "$scsitopo" > /sys/bus/scsi/drivers/sd/$scsiaction || echo "WARNING: error attempting to control block device for $dev" >> $LOGFILE;
    fi
}
    

#@desc print out a status table of the current LUN modes / mappings
#@returns sets rc to 0 on success, OR sets return code to non-zero value on error
printstatus()
{
    #list of all known SG devices - local to prevent this from causing side effects / problems in udev handler
    local _SGDEVS=`ls /sys/module/cxlflash/drivers/pci:cxlflash/*:*:*.*/host*/target*:*:*/*:*:*:*/scsi_generic | grep sg`
    local lunid=0;
    local lunmode=0;
    local blockdev=0;
    local scsitopo=0;
    echo   "CXL Flash Device Status"
    printf "%10s: %10s  %5s  %9s  %32s\n" "Device" "SCSI" "Block" "Mode" "LUN WWID";
    for dev in $_SGDEVS; do
        getlunid lunid $dev;
        getmode lunmode $dev;
        getblockdev blockdev $dev || blockdev="n/a";
        getscsitopo scsitopo $dev;
        printf "%10s: %10s, %5s, %9s, %32s\n" "$dev" "$scsitopo" "$blockdev" "$lunmode" "$lunid";
    done
}

#@desc set the mode for a given block device
#@param $1 - sg device
#@returns sets rc to 0 on success, OR sets return code to non-zero value on error
setdevmode()
{
    #$1: target sg device
    #$2: target mode
    local dev=$1;
    #causes the block device above this sg device to become mapped or unmapped
    #ctrlblockdevmap $dev $targetmode; #@TODO: disable to keep HTX happy
    echo "INFO: Setting $dev mode" >> $LOGFILE;
    #call with the config option, which causes the tool to read the config
    $CAPIKVROOT/bin/cxlflashutil -d /dev/$dev --config >> $LOGFILE;
}

#@desc Determine if a given LUN is in the SIO LUN Table
#@param $1 - output variable
#@param $2 - lunid
#@returns rc to 0 on success and sets outputvar with LUN mode of device as per table, OR sets return code to non-zero value on error
getluntablestate()
{
    local __resultvar=$1;
    local lunid=$2;
    local targetmode=$LUNMODE_LEGACY; #default to legacy

    if [[ ! -f $SIOTABLE ]]; then
        echo "Unable to access '$SIOTABLE'";
        return $ENOENT;
    fi
    #echo "searching for '$lunid'";
    #match any LUN entry that is lun=1 or lun=01
    if grep -xq "$lunid=0\?*1.\?*" $SIOTABLE ; then
        #echo "lun in SIO mode";
        targetmode=$LUNMODE_SIO; #set to SIO mode since it's in the SIO table
    fi
    eval $__resultvar="'$targetmode'"
}


#@desc walk through all cxlflash LUNs and send updates to device driver for legacy / sio mode
#@returns rc to 0 on success, OR sets return code to non-zero value on error
dotableupdate()
{
    #list of all known SG devices - local to prevent this from causing side effects / problems in udev handler
    local _SGDEVS=`ls /sys/module/cxlflash/drivers/pci:cxlflash/*:*:*.*/host*/target*:*:*/*:*:*:*/scsi_generic | grep sg`
    date >> $LOGFILE;
    if [[ ! -f $SIOTABLE ]]; then
        echo "Unable to access '$SIOTABLE'";
        return $ENOENT;
    else
        echo "SIO LUN TABLE CONTENTS:" >> $LOGFILE;
        cat $SIOTABLE >> $LOGFILE;
    fi
    echo "Refer to $LOGFILE for detailed table update logs."
    for dev in $_SGDEVS; do
        setdevmode $dev $targetmode
    done
}

