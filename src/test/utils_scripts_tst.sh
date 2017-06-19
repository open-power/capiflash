#!/bin/bash
#  IBM_PROLOG_BEGIN_TAG
#  This is an automatically generated prolog.
#
#  $Source: src/test/utils_scripts_tst.sh $
#
# IBM Data Engine for NoSQL - Power Systems Edition User Library Project
#
# Contributors Listed Below - COPYRIGHT 2014,2017
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


PATH=$PATH:/opt/ibm/capikv/bin:/opt/ibm/capikv/test:/opt/ibm/capikv/afu
BUFFER=/tmp/fvt_scripts.buffer

FAILED_CHECKS=0;TOTAL_CHECKS=0;
CMD_RC=0;
errs=0;
CMD='';TESTNAME=''


#NOOPs for Running from memory/file and when in AIX
if [[ $(uname) =~ "AIX" ]]; then
    exit 0;
fi

if [[ -z $FVT_DEV || ! $FVT_DEV =~ "/dev"  || $FVT_DEV == '' ]]; then
    echo "[  SKIPPED ] FVT_DEV is a file or is unset";
    exit 0;
fi



################################################################
#        Utility functions to help asserting conditions        #
################################################################
function echoerr {
    >&2 echo "$@"
}

function setup {
    rm -rf ${BUFFER} &>/dev/null
    CMD_RC=0;
    errs=0;
    CMD=''; TESTNAME=''
}

#expect_eq : compares two values passed in and asserts they are equal
#USAGE: expect_eq <expected_value> <actual_value> <testname>
function expect_eq {
    if [[ -z $1 || -z $2 || -z $3 ]]; then
        echo "USAGE: $0 <expected_value> <actual_value> <testname>"
        exit 5;
    fi
    TOTAL_CHECKS=$((TOTAL_CHECKS+1))
    TESTNAME=$3    
    expected=$1;
    actual=$2
    line_num=$(caller | awk '{print $1}') #line number of caller

    if [[ $expected -ne $actual ]]; then
        echoerr "ERROR($CMD): $TESTNAME @LINE_NO $line_num   - expected $expected, got $actual"
        FAILED_CHECKS=$((FAILED_CHECKS+1))
        return 1
    else
        echo "PASS ($CMD): $TESTNAME   ";
        return 0;
    fi
}

#expect_no_err_strings: parses the buffer for an error string of choice. It asserts
#                  the count is equal to zero.
#USAGE: expect_no_err_strings <filename with saved buffer> <testname> <err_str>
function expect_no_err_strings {
    if [[ -z $1 || -z $2 ]]; then
        echo "USAGE: $0 <buffer filename> <testname> <err_str>"
        exit 10 ;
    fi

    TOTAL_CHECKS=$((TOTAL_CHECKS+1))

    TESTNAME=$2;
    BUFNAME=$1
    cmp_str='';cmp_str="$3"
    errs=$(grep -c "$cmp_str" ${BUFNAME})

    line_num=$(caller | awk '{print $1}') #line number of caller
    filename=$(caller | awk '{print $2}') #file name    
    if [[ 0 -ne $errs ]]; then
        echoerr "ERROR($CMD): $TESTNAME @LINE_NO $line_num   - found $errs \"$cmp_str\" strings" 
        FAILED_CHECKS=$((FAILED_CHECKS+1)); 
        return 1
    else
        echo "PASS ($CMD): $TESTNAME";
        return 0
    fi
}

#expect_str: parses the buffer for a particular string passed in.
#USAGE: expect_str <filename with saved buffer> <testname> <str to compare>
function expect_str {

    if [[ -z $1 || -z $2 || -z $3 ]]; then
        echo "USAGE: expect_no_err_strings <buffer filename> <testname> <str to compare>"
        exit 15 ;
    fi

    TOTAL_CHECKS=$((TOTAL_CHECKS+1))
    cmp_str='';cmp_str=$3;
    TESTNAME=$2;
    BUFNAME=$1
    errs=$(grep -c "$cmp_str" ${BUFNAME})

    line_num=$(caller | awk '{print $1}') #line number of caller

    if [[ $errs -lt 1 ]]; then
        echoerr "ERROR($CMD): $TESTNAME @LINE_NO $line_num   - found $errs \"$cmp_str\" strings"
        FAILED_CHECKS=$((FAILED_CHECKS+1))
    else
        echo "PASS ($CMD): $TESTNAME";
    fi

}

#print_summary: prints out statistics of total checks run vs failures.
function print_summary {
    END_DATE=$(date);
    END_TIME=$(date +%H%M)

    #echo "$END_TIME - $START_TIME" 
    ELAPSED_MINS=0;
    shr=$((10#$START_TIME/100));
    smin=$((10#$START_TIME%(shr*100)));
    ehr=$((10#$END_TIME/100));
    emin=$((10#$END_TIME%(shr*100)));
    ELAPSED_MINS=$(( (ehr*60 + emin) - (shr*60+smin) ))

    printf "\n\n"
    printf "****************************************************************\n"
    printf "* Finished tests at %s\n" "${END_DATE}"
    printf "* Elapsed mins: %-46s *\n" "$ELAPSED_MINS"
    printf "*                      TEST RESULTS                            *\n"
    printf "****************************************************************\n"
    printf "*  TEST_PASSED: %s of %s\n" "$((TOTAL_CHECKS-FAILED_CHECKS))" "$TOTAL_CHECKS"
    printf "*  TEST_FAILED: %s of %s\n" "$FAILED_CHECKS" "$TOTAL_CHECKS"
    printf "\n\n"

    rm -rf $BUFFER
}

trap "print_summary;" EXIT 

################################################################
#                Test case functions                           #
################################################################

################################################################
#Print CXL Flash Device Status
################################################################
function CXLFSTATUS_RC_TEST {
    setup;CMD=cxlfstatus
    ${CMD} &> $BUFFER;
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CXLFSTATUS_RC_TEST"
}

function CXLFSTATUS_ERRSTR_TEST {
    setup;CMD=cxlfstatus
    ${CMD} &> $BUFFER;
    expect_no_err_strings $BUFFER "CXLFSTATUS_ERRSTR_TEST" "ERROR[: ]";
    expect_no_err_strings $BUFFER "CXLFSTATUS_ERRSTR_TEST" "error[: ]";    
}

################################################################
#Run cflash_version test for ERROR outputs
################################################################
function CFLVER_RC_TEST {
    setup; CMD=cflash_version;
    ${CMD}  &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLVER_RC_TEST"
}

function CFLVER_ERRSTR_TEST {
    setup; CMD=cflash_version;
    ${CMD}  &> $BUFFER
    expect_no_err_strings $BUFFER "CFLVER_ERRSTR_TEST" "ERROR[: ]";
    expect_no_err_strings $BUFFER "CFLVER_ERRSTR_TEST" "error[: ]";    
}


################################################################
#Run capi_flash test for "Found CAPI"
################################################################
function CFLASH_RC_TEST {
    setup; CMD=capi_flash.pl;
    ${CMD} -l &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_RC_TEST"

}

function CFLASH_FIND_CAPI_TEST {
    setup; CMD=capi_flash.pl;
    ${CMD} -l &> $BUFFER
    expect_str $BUFFER "CFLASH_FIND_CAPI_TEST" "Found CAPI";
}

################################################################
#Run capi_flash.pl searching for -help screen
################################################################
function CFLASH_USAGE_TEST {
    setup; CMD=capi_flash.pl;
    ${CMD} &> $BUFFER
    expect_str $BUFFER "CFLASH_USAGE_TEST" "Usage";
}

################################################################
#Run cablecheck 
################################################################
function CBLCHK_RC_TEST {
    setup; CMD=cablecheck
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CBLCHK_RC_TEST"
}

function CBLCHK_ERRSTR_TEST {
    setup; CMD=cablecheck
    ${CMD} &> $BUFFER
    expect_no_err_strings $BUFFER "CBLCHK_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CBLCHK_ERRSTR_TEST" "error[: ]"
}

################################################################
#Run cxlffdc
################################################################
function CXLFFDC_RC_TEST {
    setup; CMD=cxlffdc
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CXLFFDC_RC_TEST"

}

function CXLFFDC_ERRSTR_TEST {
    setup; CMD=cxlffdc
    ${CMD} &> $BUFFER
    expect_no_err_strings $BUFFER "CXLFFDC_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CXLFFDC_ERRSTR_TEST" "error[: ]"
}

################################################################
#Run cxlfrefreshluns
################################################################
function CXLREFRESH_RC_TEST {
    setup; CMD=cxlfrefreshluns
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CXLREFRESH_RC_TEST"
}

function CXLREFRESH_ERRSTR_TEST {
    setup; CMD=cxlfrefreshluns
    ${CMD} &> $BUFFER
    expect_no_err_strings $BUFFER "CXLREFRESH_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CXLREFRESH_ERRSTR_TEST" "error[: ]"
}

################################################################
#Run cflash_perfcheck.pl
################################################################
function CFLASH_PERFCHK_RC_TEST {
    setup; CMD=cflash_perfcheck.pl

    superpipe_dev=$(cxlfrefreshluns | grep "superpipe" | cut -d: -f1 | sed -n 1p)
    dev='';dev=$(cflash_devices.pl | grep $superpipe_dev )

    if [[ -z $dev || -z $superpipe_dev ]]; then
        echoerr "${CMD} no dev found";
        FAILED_CHECKS=$((FAILED_CHECKS+1)); TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi
    ${CMD} -d $dev -s 2 &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CMD_RC_CHECK_TEST"
}

function CFLASH_PERFCHK_ERRSTR_TEST {
    setup; CMD=cflash_perfcheck.pl
    dev='';dev=$(cflash_devices.pl | sed -n 1p)    
    superpipe_dev=$(cxlfrefreshluns | grep "superpipe" | cut -d: -f1 | sed -n 1p)

    if [[ -z $dev || -z $superpipe_dev ]]; then
        echoerr "${CMD} no dev found";
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi

    ${CMD} -d $dev -s 2 &> $BUFFER
    expect_no_err_strings $BUFFER "CFLASH_PERFCHK_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CFLASH_PERFCHK_ERRSTR_TEST" "error[: ]"
}

################################################################
#Run cflash_capacity.pl
################################################################
function CFLASH_CAP_RC_TEST {
    setup; CMD=cflash_capacity.pl
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_CAP_RC_TEST"

}

function CFLASH_CAP_ERRSTR_TEST {
    setup; CMD=cflash_capacity.pl
    ${CMD} &> $BUFFER
    expect_no_err_strings $BUFFER "CFLASH_CAP_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CFLASH_CAP_ERRSTR_TEST" "error[: ]"

}

################################################################
#Run cflash_devices.pl and check for at least one sg* dev
################################################################
function CFLASH_DEV_RC_TEST {
    setup; CMD=cflash_devices.pl
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_DEV_RC_TEST"
}

function CFLASH_DEV_FIND_SG_TEST {
    setup; CMD=cflash_devices.pl
    ${CMD} &> $BUFFER
    expect_str $BUFFER "CFLASH_DEV_FIND_SG_TEST" "/dev/sg*"
}

################################################################
##Run cflash_temp.pl and check for at least one sg* dev
################################################################
function CFLASH_TEMP_RC_TEST {
    setup; CMD=cflash_temp.pl
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_TEMP_RC_TEST"
}

function CFLASH_TEMP_FIND_DEV {
    setup; CMD=cflash_temp.pl
    ${CMD} &> $BUFFER
    expect_str $BUFFER "CFLASH_TEMP_FIND_DEV" "Found"
}

#################################################################
##Run machine_info and check for at least one sg* dev
#################################################################
function MACHINE_INFO_RC_TEST {
    setup; CMD=machine_info
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "MACHINE_INFO_RC_TEST"
}

function MACHINE_INFO_ERRSTR_TEST {
    setup; CMD=machine_info
    ${CMD} &> $BUFFER
    expect_no_err_strings $BUFFER "MACHINE_INFO_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "MACHINE_INFO_ERRSTR_TEST" "error[: ]"
}

################################################################
#Run reload_all_adapters. check at least one was successfull, no
#timeouts occurred
################################################################
function RELOAD_ADAPS_RC_TEST {
    setup; CMD=reload_all_adapters
    ${CMD} &> $BUFFER
    rc=$?
    expect_eq 0 $rc "RELOAD_ADAPS_RC_TEST"
}

function RELOAD_ADAPS_SUCC_TEST {
    setup; CMD=reload_all_adapters
    ${CMD} &> $BUFFER
    #at least one of these str
    expect_str $BUFFER "RELOAD_ADAPS_SUCC_TEST" "successfully loaded user"  
    expect_no_err_strings $BUFFER "RELOAD_ADAPS_NO_TIMEOUT" "timeout";
}

################################################################
#Run reload_all_adapters. check at least one was successfull, no
#timeouts occurred
################################################################
function RELOAD_ADAPS_RC_TEST {
    setup; CMD=reload_all_adapters
    ${CMD} &> $BUFFER
    rc=$?
    expect_eq 0 $rc "RELOAD_ADAPS_RC_TEST"
}

function RELOAD_ADAPS_SUCC_TEST {
    setup; CMD=reload_all_adapters
    ${CMD} &> $BUFFER
    #at least one of these str
    expect_str $BUFFER "RELOAD_ADAPS_SUCC_TEST" "successfully loaded user"  
    expect_no_err_strings $BUFFER "RELOAD_ADAPS_NO_TIMEOUT" "timeout";
}

################################################################
#Run cflash_wear.pl
################################################################
function CFLASH_WEAR_RC_TEST {
    setup; CMD=cflash_wear.pl
    ${CMD} &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_WEAR_RC_TEST"

}

function CFLASH_WEAR_ERRSTR_TEST {
    setup; CMD=cflash_wear.pl
    ${CMD} &> $BUFFER
    expect_no_err_strings $BUFFER "CFLASH_WEAR_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CFLASH_WEAR_ERRSTR_TEST" "error[: ]"
    expect_no_err_strings $BUFFER "CFLASH_WEAR_ERRSTR_TEST" "100%"

}

function CFLASH_WEAR_FOUND_TEST {
    setup; CMD=cflash_wear.pl
    ${CMD} &> $BUFFER
    expect_str $BUFFER "CFLASH_WEAR_FOUND_TEST" "Found 0601"
    expect_str $BUFFER "CFLASH_WEAR_FOUND_TEST" "sg[0-9]"
}

################################################################
#Run cxlfsetlunmode
################################################################
function CXLFSETLUNMODE_RC_TEST {
    setup; CMD=cxlfsetlunmode
    lun='';lun=$(cxlfstatus  | grep sg | cut -d, -f 4 | sed -n 1p)

    if [[ -z $lun ]]; then
        echoerr "No lun found"
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return ;
    fi

    mode=""; mode=$(cxlfrefreshluns | grep $lun |cut -d, -f3)

    if [[ -z $mode ]]; then
        echoerr "No lun mode found"
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi

    if [[ "$mode" =~ "legacy" ]]; then
       orig_mode=0; new_mode=1
    else
        orig_mode=1;new_mode=0
    fi

    ${CMD} $lun $new_mode  &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CXLFSETLUNMODE_RC_TEST"

    #return to original mode
    setup; CMD=cxlfsetlunmode
    ${CMD} $lun $orig_mode  &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CXLFSETLUNMODE_RC_TEST"

}

function CXLFSETLUNMODE_ERRSTR_TEST {
    setup; CMD=cxlfsetlunmode

    lun='';lun=$(cxlfstatus  | grep sg | cut -d, -f 4 | sed -n 1p)
    if [[ -z $lun ]]; then
        echoerr "No lun found"
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return ;
    fi

    mode="legacy"; mode=$(cxlfrefreshluns | grep $lun |cut -d, -f3)

    if [[ -z $mode ]]; then
        echoerr "No lun mode found"
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi

    if [[ "$mode" =~ "legacy" ]]; then
        orig_mode=0; new_mode=1
    else
        orig_mode=1;new_mode=0
    fi


    ${CMD} $lun $new_mode &> $BUFFER
    expect_no_err_strings $BUFFER "CXLFSETLUNMODE_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CXLFSETLUNMODE_ERRSTR_TEST" "error[: ]"
    expect_str $BUFFER "CXLFSETLUNMODE_SUCC_STR_TEST" "SUCCESS"

    setup; CMD=cxlfsetlunmode
    ${CMD} $lun $orig_mode &> $BUFFER
    expect_str $BUFFER "CXLFSETLUNMODE_SUCC_STR_TEST" "SUCCESS"
}

################################################################
#Run cflash_stick.pl
################################################################
function CFLASH_STILST_RC_TEST {
    setup; CMD=cflash_stick.pl
    ${CMD} -l &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_STILST_RC_TEST"

}

function CFLASH_STILST_ERRSTR_TEST {
    setup; CMD=cflash_stick.pl
    ${CMD} -l &> $BUFFER
    expect_no_err_strings $BUFFER "CFLASH_STILST_ERRSTR_TEST" "ERROR[: ]"
    expect_no_err_strings $BUFFER "CFLASH_STILST_ERRSTR_TEST" "error[: ]"
    expect_str $BUFFER "CFLASH_STILST_SUCC_STR_TEST" "dev/sg*"

}

################################################################
#Run cflash_perst.pl
################################################################
function CFLASH_PERST_RC_TEST {
    setup; CMD=cflash_perst.pl

    slot=$(capi_flash.pl -l | grep 0601 | sed -n 1p |awk '{print $6}')
    if [[ -z $slot ]]; then
        echoerr "Slot not found."
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi

    rmmod cxlflash 
    if [[ $? -ne 0 ]]; then echoerr "[  SKIPPED ]"; return; fi
    ${CMD} -t $slot &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_PERST_RC_TEST"
    modprobe cxlflash 
}

function CFLASH_PERST_ERRSTR_TEST {
    setup; CMD=cflash_perst.pl

    slot=$(capi_flash.pl -l | grep 0601 | sed -n 1p |awk '{print $6}')
    if [[ -z $slot ]]; then
        echoerr "Slot not found."
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi

    rmmod cxlflash
    if [[ $? -ne 0 ]]; then echoerr "[  SKIPPED ]"; return; fi
    ${CMD} -t $slot &> $BUFFER
    expect_no_err_strings $BUFFER "CFLASH_PERST_ERRSTR_TEST" "ERROR[: ]"
    expect_str $BUFFER "CFLASH_PERST_SUCC_STR_TEST" "$slot"
    expect_str $BUFFER "CFLASH_PERST_SUCC_STR_TEST" "successfully"

    modprobe cxlflash 
}

################################################################
#Run cflash_reset.pl
################################################################
function CFLASH_RESET_RC_TEST {
    setup; CMD=cflash_reset.pl
 
    slot=$(capi_flash.pl -l | grep 0601 | sed -n 1p |awk '{print $6}')
    if [[ -z $slot ]]; then
        echoerr "Slot not found."
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi

    ${CMD} -t $slot &> $BUFFER
    CMD_RC=$?
    expect_eq 0 $CMD_RC "CFLASH_RESET_RC_TEST"
}

function CFLASH_RESET_ERRSTR_TEST {
    setup; CMD=cflash_reset.pl

    slot=$(capi_flash.pl -l | grep 0601 | sed -n 1p |awk '{print $6}')
    if [[ -z $slot ]]; then
        echoerr "Slot not found."
        FAILED_CHECKS=$((FAILED_CHECKS+1));TOTAL_CHECKS=$((TOTAL_CHECKS+1))
        return;
    fi

    ${CMD} -t $slot &> $BUFFER
    expect_no_err_strings $BUFFER "CFLASH_RESET_ERRSTR_TEST" "ERROR[: ]"
}


################################################################
#            Script options and running of tests               #
################################################################


function usage () { echo "Usage: $0 [-t <TESTNAME>]"; exit 20; }

TESTFUNCTION=''
while getopts "t:" opt ; do
    case "${opt}" in
        t) # expect a TESTNAME
            #echo "-t  $OPTARG"
            #Is it a valid function/testname ?
            if [[ -n "$(type -t $OPTARG)" && "$(type -t $OPTARG)" == "function" ]];
            then
#                echo "TESTFUNCTION found: $OPTARG"
                TESTFUNCTION="$OPTARG"
            else
                echo "ERROR: Invalid test name - $OPTARG"
                START_TIME=$(date +%H%M)
                exit 25
            fi
            break;
            ;;
        \? | :)
            usage;
            ;;

    esac
done

################################################################
#                 Run of test cases                            #
################################################################
START_TIME=$(date +%H%M)

#cflash_perst.pl

if [[ -n $TESTFUNCTION ]]; then
    #execute single test
    ${TESTFUNCTION}
else #execute all the tests
    CXLFSTATUS_RC_TEST
    CXLFSTATUS_ERRSTR_TEST
    CFLVER_RC_TEST
    CFLVER_ERRSTR_TEST
    CFLASH_RC_TEST
    CFLASH_FIND_CAPI_TEST
    CFLASH_USAGE_TEST
    #    CBLCHK_RC_TEST
    #   CBLCHK_ERRSTR_TEST
    CXLFFDC_RC_TEST
    CXLFFDC_ERRSTR_TEST
    CXLREFRESH_RC_TEST
    CXLREFRESH_ERRSTR_TEST
    CFLASH_PERFCHK_RC_TEST 
    CFLASH_PERFCHK_ERRSTR_TEST
    CFLASH_CAP_RC_TEST
    CFLASH_CAP_ERRSTR_TEST
    CFLASH_DEV_RC_TEST
    CFLASH_DEV_FIND_SG_TEST
    CFLASH_TEMP_RC_TEST
    CFLASH_TEMP_FIND_DEV
    MACHINE_INFO_RC_TEST
    MACHINE_INFO_ERRSTR_TEST
    #disabled as per Mike V. request
    # RELOAD_ADAPS_RC_TEST 
    # RELOAD_ADAPS_SUCC_TEST
    CFLASH_WEAR_RC_TEST
    CFLASH_WEAR_ERRSTR_TEST
    CFLASH_WEAR_FOUND_TEST
    CXLFSETLUNMODE_ERRSTR_TEST
    CXLFSETLUNMODE_RC_TEST
    exit $FAILED_CHECKS
fi

#if any checks failed return as failure of test cases
if [[ $FAILED_CHECKS -eq 0 ]]; then 
    exit 0
else
    exit 1
fi
