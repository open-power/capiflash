#!/bin/bash
#  IBM_PROLOG_BEGIN_TAG
#  This is an automatically generated prolog.
#
#  $Source: src/build/install/resources/afucfg.sh $
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

#run capi-specific adapter configs
echo "INFO: Running adapter-specific configuration."

accels=`lspci -d 1014:04cf | awk '{print $1}'`
for accel in ${accels}; do
    setpci -s ${accel} 808.L=440000
    readback=`setpci -s ${accel} 808.L`
    echo "INFO: Accelerator ${accel} cfg register 808: ${readback}"
done 

