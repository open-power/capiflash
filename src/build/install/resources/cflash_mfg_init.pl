#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/install/resources/cflash_mfg_init.pl $
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

#-------------------------------------------------------------------------------
# Variables
my $target;
my $prthelp=0;
my $serial;

sub usage()
{
  print "\n";
  print "Usage: [-t target] [-s serial#]                               \n";
  print "    -t or --target : pci target                               \n";
  print "    -s or --serial : 8 digit serial number                    \n";
  print "    -h or --help   : Help - Print this message                \n";
  print "\n  ex: -t 0000:01:00.0 -s 12345678                         \n\n";
  exit 0;
}

#check sudo permissions
(`id -u` == 0) || die "Run with sudo permissions\n";

#-------------------------------------------------------------------------------
# Parse Options
#-------------------------------------------------------------------------------
if ($#ARGV<0) {usage();}

if (! GetOptions ("t|target=s" => \$target,
                  "s|serial:s" => \$serial,
                  "h|help!"    => \$prthelp
                  ))
{
  usage();
}
if ($ARGV[0])
{
  print "\nUnknown Command Line Options:\n";
  foreach(@ARGV)
  {
    print "   $_\n";
  }
  print "\n";
  usage();
}
if ($prthelp) {usage();}

if (defined $serial) {(length($serial)==8) || die "serial is not 8 chars\n";}

#-------------------------------------------------------------------------------
# Make stdout autoflush
#-------------------------------------------------------------------------------
select(STDOUT);
$| = 1;

my $out=`lspci -s $target |grep 0601`;
($out =~ /0601/) || die "Invalid target $target\n";

my $afu=`ls -d /sys/devices/pci*/*/$target/cxl/card*/afu* 2>/dev/null|awk -F/ '{print \$NF}'`;
($afu =~ /afu/) || die "Invalid target $target\n";
chomp $afu;
$afu .= 'm';

my $card=`ls -d /sys/devices/pci*/*/$target/cxl/card* 2>/dev/null|awk -F/ '{print \$NF}'`;
($card =~ /card/) || die "Invalid target $target\n";
chomp $card;
my $cardN=substr $card,-1;

if (defined $serial)
{
  chdir('/opt/ibm/capikv/afu') || die "cxlflashimage pkg is not installed\n";
  `./surelock_vpd2rbf.pl -f vpd_FlashGT.csv -s $serial`;
  (-f "/opt/ibm/capikv/afu/vpd_FlashGT.vpdrbf") || die "failure creating the VPD\n";
  `/opt/ibm/capikv/afu/flashgt_vpd_access $target $cardN /opt/ibm/capikv/afu/vpd_FlashGT.vpdrbf`;
  if ($? != 0) {print "VPD update failed\n";}
  else         {print "VPD updated\n";}
}

if (defined $target)
{
  `/opt/ibm/capikv/afu/cxl_afu_fmt_unit -p0 /dev/cxl/$afu`;
  ($? == 0) || die "format lun 0 failed\n";
  print "lun 0 cleaned\n";
  `/opt/ibm/capikv/afu/cxl_afu_fmt_unit -p1 /dev/cxl/$afu`;
  ($? == 0) || die "format lun 1 failed\n";
  print "lun 1 cleaned\n";
}

exit 0;

