#!/bin/bash
#  IBM_PROLOG_BEGIN_TAG
#  This is an automatically generated prolog.
#
#  $Source: src/build/install/resources/cflashutils.sh $
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

SIOTABLE=/etc/cxlflash/sioluntable.ini
LOGFILE=/tmp/cxlflog.${USER}.log

#cxlflash LUN modes
LUNMODE_LEGACY=0;
LUNMODE_SIO=1;

#common return codes
ENOENT=2;
EIO=5;
EACCES=13;
EINVAL=22;

_VM=$(cat /proc/cpuinfo|grep platform|grep pSeries)

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
            echo "INFO: LUN $targetlun has unknown LUN Status ($currmode) or target mode ($targetmode)."
            return $EINVAL;
        ;;
    esac

    #do bind/unbind if required
    local _SGDEVS=`ls /sys/module/cxlflash/drivers/pci:cxlflash/*:*:*.*/host*/target*:*:*/*:*:*:*/scsi_generic 2>/dev/null| grep sg`
    local wwid
    for dev in $_SGDEVS; do
        getlunid wwid $dev
        if [[ $wwid = $targetlun ]]; then
          ctrlblockdevmap $dev $targetmode
        fi
    done
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
        echo "INFO: Invalid sg device: $dev"
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
    local block="   ";
    if [[ -e /sys/class/scsi_generic/$dev/device/block ]]; then
        local block=`ls /sys/class/scsi_generic/$dev/device/block`;
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
    local blockdev=;
    getblockdev blockdev $dev;
    getscsitopo scsitopo $dev;
    #only unmap the device if the block device is NOT unknown, and we want to go to SIO mode
    if [[ ( "$tgmode" == "$LUNMODE_SIO" ) &&  ( "$blockdev" != "   " ) ]]; then
        scsiaction="unbind";
    #only map the device if the block device is unknown, and want to go to LEGACY mode
    elif [[ ("$tgmode" == "$LUNMODE_LEGACY") &&  ("$blockdev" == "   ") ]]; then
        scsiaction="bind";
    fi
    echo "INFO: sgdev:$dev mode:$tgmode action:${scsiaction} sddev:$blockdev" >> $LOGFILE;
    #if there's something to do, call either bind or unbind appropriately
    if [[ "$scsiaction" != "noop" && $blockdev ]]; then
        out=$(echo -n "$scsitopo" > /sys/bus/scsi/drivers/sd/$scsiaction 2>&1)
        echo "INFO: action:${scsiaction} sgdev:$dev sddev:$blockdev scsitopo:$scsitopo rc:$?" >> $LOGFILE;
    fi
}

#@desc print out a status table of the current LUN modes / mappings
#@returns sets rc to 0 on success, OR sets return code to non-zero value on error
printstatus()
{
    declare -A GMAP
    declare -A DMAP
    echo   "CXL Flash Device Status"
    if [[ -z $_VM ]]
    then
      _capis=$(lspci|egrep "0628|0601|04cf" 2>/dev/null|awk '{print $1}')
    else
      _capis=$(ls -d /sys/bus/pci/drivers/cxlflash/*:* 2>/dev/null|awk -F/ '{print $7}')
    fi

    for capi in $_capis
    do
      if [[ -z $_VM ]]
      then
        devid=$(lspci|grep $capi |awk -F" " '{print $6}')
      else
        grep_id=$(echo $capi|cut -c6-)
        devid=$(lspci|grep $grep_id |awk -F" " '{print $7}')
      fi
      local devspec=$(cat /sys/bus/pci/devices/$capi/devspec)
      local loccode=$(cat /proc/device-tree$devspec/ibm,loc-code 2>/dev/null)

      printf "\nFound $devid $capi $loccode\n"

      #list of all known SG devices - local to prevent this from causing side effects / problems in udev handler
      if [[ -z $_VM ]]
      then
        _SGDEVS=$(ls -d /sys/devices/pci*/*/$capi/pci*/*/host*/target*/*/scsi_generic/sg* 2>/dev/null|awk -F/ '{print $13}')
      else
         _SGDEVS=$(ls -d /sys/devices/platform/*ibm,coherent-platform-facility/*/$capi/*/*/*/scsi_generic/sg* 2>/dev/null |awk -F/ '{print $12}'|sort -V)
      fi

      local lunid=0;
      local lunmode=0;
      local blockdev=0;
      local scsitopo=0;
      if [[ -z $_SGDEVS ]]
      then
        continue
      fi
      sgdevs=$(ls -1 /dev|grep sg)
      sddevs=$(ls -1 /dev|grep sd)
      for dev in $sgdevs; do link=$(readlink /dev/$dev); if [[ ! -z $link ]]; then GMAP[$link]=$dev; sgpdevs=1; fi; done
      for dev in $sddevs; do link=$(readlink /dev/$dev); if [[ ! -z $link ]]; then DMAP[$link]=$dev; sdpdevs=1; fi; done

      printf "  %-8s %-10s %-8s %-10s %-33s %-s" "Device:" "SCSI" "Block" "Mode" "LUN WWID"
      if [[ $sgpdevs || $sdpdevs ]]; then printf "Persist"; fi
      printf "\n";
      for dev in $_SGDEVS
      do
          getlunid lunid $dev;
          getmode lunmode $dev;
          getblockdev blockdev $dev;
          getscsitopo scsitopo $dev;
          printf "  %-8s %-10s %-8s %-10s %-34s" "$dev:" "$scsitopo," "$blockdev," "$lunmode," "$lunid,"
          if [[ $sgpdevs ]]; then printf "%-11s" "${GMAP[$dev]},";      fi
          if [[ $sdpdevs ]]; then printf "%-11s" "${DMAP[$blockdev]}"; fi
          printf "\n"
      done
    done
    printf "\n"
}

#@desc set the mode for a given block device
#@param $1 - sg device
#@returns sets rc to 0 on success, OR sets return code to non-zero value on error
setdevmode()
{
    #$1: target sg device
    #$2: target mode
    local dev=$1;
    local targetmode=$2
    #causes the block device above this sg device to become mapped or unmapped
    ctrlblockdevmap $dev $targetmode
    echo "INFO: Setting $dev mode:$targetmode" >> $LOGFILE;
    #call with the config option, which causes the tool to read the config
    /usr/bin/cxlflashutil -d /dev/$dev --config >> $LOGFILE;
}

#@desc Determine if a given LUN is in the SIO LUN Table
#@param $1 - output variable
#@param $2 - lunid
#@returns rc to 0 on success and sets outputvar with LUN mode of device as per table, OR sets return code to non-zero value on error
getluntablestate()
{
    local __resultvar=$1;
    local wwid=$2;
    local targetmode=$LUNMODE_LEGACY; #default to legacy

    if [[ ! -f $SIOTABLE ]]; then
        echo "ERROR: Unable to access '$SIOTABLE'";
        return $ENOENT;
    fi
    #match any LUN entry that is lun=1 or lun=01
    if [[ $(grep $wwid $SIOTABLE | awk -F= '{print $2}') -eq 1 ]]; then
        targetmode=$LUNMODE_SIO;
    fi
    eval $__resultvar=$targetmode
}


#@desc walk through all cxlflash LUNs and send updates to device driver for legacy / sio mode
#@returns rc to 0 on success, OR sets return code to non-zero value on error
dotableupdate()
{
    #check sudo permissions
    SUDO=$(id -u)
    if [[ $SUDO != 0 ]]; then echo "run with sudo permissions to refresh"; return; fi

    #attempt to rescan for cxl scsi devices
    local host;
    for host in $(ls /sys/module/cxlflash/drivers/pci:cxlflash/*:*:*.*/ 2>/dev/null| grep host)
    do
       echo "- - -" > /sys/class/scsi_host/$host/scan
    done

    #list of all known SG devices - local to prevent this from causing side effects / problems in udev handler
    local _SGDEVS=`ls /sys/module/cxlflash/drivers/pci:cxlflash/*:*:*.*/host*/target*:*:*/*:*:*:*/scsi_generic 2>/dev/null| grep sg`
    date >> $LOGFILE;
    if [[ ! -f $SIOTABLE ]]; then
        echo "ERROR: Unable to access '$SIOTABLE'";
        return $ENOENT;
    else
        echo "SIO LUN TABLE CONTENTS:" >> $LOGFILE;
        cat $SIOTABLE >> $LOGFILE;
    fi
    echo "Refer to $LOGFILE for detailed table update logs."

    local mode
    local wwid
    for dev in $_SGDEVS; do
        getlunid wwid $dev
        getluntablestate mode $wwid
        setdevmode $dev $mode
    done
}

