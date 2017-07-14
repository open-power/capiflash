#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/install/resources/cflash_temp.pl $
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
##
use strict;
use warnings;
use Fcntl;
use Fcntl ':seek';
use Getopt::Long;

my $cmd="";
my $out;
my $ubuntu=0;
my $redhat=0;

#determine distro
if ( `cat /etc/redhat-release 2>/dev/null` =~ "Red Hat" ) {$redhat=1;}
if ( `lsb_release -a 2>/dev/null` =~ "Ubuntu" ) {$ubuntu=1;}

#check sudo permissions
(`id -u` == 0) || die "Run with sudo permissions";

#ensure sg_write_buffer is available
if ( $redhat ) {$cmd="rpm -qa | grep -v udev|grep -v libs | grep sg3_utils >/dev/null "; }
if ( $ubuntu ) {$cmd="dpkg -l|grep -v udev|grep sg3-utils >/dev/null ";}

system($cmd);
if ($? != 0) {print "ERROR: package sg3-utils is not installed\n"; exit -1;}

# Make stdout autoflush
select(STDOUT);
$| = 1;

# print temps
my @cards = `lspci |egrep "0601|0628"`;

for my $adap (@cards)
{
  my @Acard=split / /, $adap;
  my $card=$Acard[0];
  chomp $card;
  my $afustr=`ls -d /sys/devices/*/*/$card/cxl/card*/afu*.0/afu*m 2>/dev/null`;
  my @afu=split /\//, $afustr;
  my $afu=pop @afu;
  if (length $afu) {chomp $afu;}
  else             {$afu="";}
  my $cardstr=`ls -d /sys/devices/*/*/$card/cxl/card* 2>/dev/null`;
  chomp $cardstr;
  my $cardN=substr $cardstr,-1;
  print "\nFound $Acard[5] $card $afu\n";
  $out=`/usr/bin/flashgt_temp $card $cardN`;
  print "  $out";

  for (my $i=0; $i<2; $i++)
  {
    my $Astick=`ls -d /sys/devices/*/*/$card/*/*/host*/target*:$i:*/*:*:*:*/scsi_generic/* 2>/dev/null`;
    if ($? != 0) {next;}
    my @sdev=split /\//, $Astick;
    my $dev=$sdev[12];
    chomp $dev;
    $out=`sg_logs -p 0xd /dev/$dev |grep "Current temp"`;
    print "  $dev: $out";
  }
}

