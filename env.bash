# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: env.bash $
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

export CUSTOMFLAGS=
export BLOCK_FILEMODE_ENABLED=
export BLOCK_MC_ENABLED=
export TARGET_PLATFORM=
export BLOCK_KERNEL_MC_ENABLED=

#allow a user to specify a custom RC file if needed
#e.g. disable the advanced toolchain with "export USE_ADVANCED_TOOLCHAIN=no"
if [ -e ./customrc ]; then
    echo "INFO: Running customrc"
    set -x
    . ./customrc
    set +x
fi

##  setup git hooks for this session
##   adds prologs and Change-IDs for gerrit
export SURELOCKROOT=`pwd`
TOOLSDIR=${SURELOCKROOT}/src/build/tools
if [ -e $TOOLSDIR/setupgithooks.sh ]; then
    echo "Setting up gerrit hooks."
    $TOOLSDIR/setupgithooks.sh
fi


export MCP_PATH=/opt/mcp/toolchains/fr_SL1_2014-05-12-194021

#configure advanced toolchain for linux
AT71PATH=/opt/at7.1
AT90PATH=/opt/at9.0-2-rc2
ATPATH=/opt/at10.0

if [ -d $MCP_PATH ]; then
    echo "INFO: Found MCP: $MCP_PATH ."
    echo "INFO: Enabling JAILchain for builds."
    export JAIL=ppc64-mcp75-jail
else
    echo "INFO: MCP Jail disabled."
fi



if [ -d $ATPATH ]; then
    export ADV_TOOLCHAIN_PATH=$ATPATH
else
    echo "WARNING: no toolchain was found. Will fall back to system defaults. YMMV."
fi

#don't add MCP path to the $PATH... this isn't absolutely necessary
#export PATH=${MCP_PATH}/opt/mcp/bin:${MCP_PATH}/usr/bin:${PATH}
export PATH=/opt/mcp/bin:${PATH}

export PATH=${PATH}:`pwd`/src/build/tools



#enable advanced toolchain, if no one has an opinion
if [ -z "$USE_ADVANCED_TOOLCHAIN" ]; then
    #enabling advanced toolchain by default. If you don't want this, set USED_ADVANCED_TOOLCHAIN in your environment
    export USE_ADVANCED_TOOLCHAIN=yes
fi
if [ "$USE_ADVANCED_TOOLCHAIN" = "yes" ]; then
    echo "INFO: Enabling Advanced Toolchain: $ADV_TOOLCHAIN_PATH"
    export PATH=${ADV_TOOLCHAIN_PATH}/bin:${ADV_TOOLCHAIN_PATH}/sbin:${PATH}
else
    echo "INFO: Advanced Toolchain Disabled."
fi


#fix up sandboxes in ODE, if we need to
if [ -n "${SANDBOXROOT}" ]; then
    if [ -n "${SANDBOXNAME}" ]; then
        export SANDBOXBASE="${SANDBOXROOT}/${SANDBOXNAME}"
    fi
fi

#set the default ulimit -c for a developer
ulimit -c unlimited
