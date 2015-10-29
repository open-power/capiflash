#!/bin/bash
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/master/test/mc_signal.bash $
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

i=1

#Check if user is root
if [ $(whoami) != 'root' ]
then
	echo "Signals should not kill mserv"
	while [ "$i" -le 64 ]
	do
		echo "$i sending SIG"`kill -l $i`" to mserv process"
		pkill -$i mserv
		pidof mserv
		if [ `echo $?` -eq 1 ]
		then
			echo "mserv killed by non root user with signal SIG"`kill -l $i`
			exit 1
		fi
		sleep 1
		i=$[$i+1]
	done

else
	while [ "$i" -le 64 ]; 
	do
  	echo "$i sending SIG"`kill -l $i`" to mserv process"
		pkill -$i mserv
		echo "Waiting"
		sleep 8
		pidof mserv

		if [ `echo $?` -eq 1 ] 
		then 
			echo "mserv not running" 
			ps -ef | grep msev | grep -v grep
			exit 1
		else 
			echo "mserv running" 
		fi

		i=$[$i+1]
		sleep 2
	done
fi
