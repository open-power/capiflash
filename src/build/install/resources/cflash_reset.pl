#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/install/resources/cflash_reset.pl $
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
#-------------------------------------------------------------------------------
my $target="";
my $prthelp=0;
my $v="";
my $rc;
my $redhat;
my $force;

my $VM=0;
if ( `cat /proc/cpuinfo | grep platform 2>/dev/null` =~ "pSeries" ) { $VM=1;}
if ( $VM ) { print "\nNot Supported on VM\n\n"; exit -1;}

sub runcmd
{
  my $cmd=shift;
  my $rc=0;
  system($cmd); $rc=$?;
  if ($rc != 0)
  {
    print "CMD_FAIL: ($cmd), rc=$rc\n";
    exit -1;
  }
}

sub usage()
{
  print "\n";
  print "Usage: cflash_reset.pl [-t <target>]                              \n";
  print "    -t or --target : Target device to perst                       \n";
  print "    -h or --help   : Help - Print this message                    \n";
  print "\n  ex: cflash_reset.pl -t 000N:01:00.0                         \n\n";
  exit 0;
}

#-------------------------------------------------------------------------------
# Parse Options
#-------------------------------------------------------------------------------
if ($#ARGV<0) {usage();}

if (! GetOptions ("t|target=s" => \$target,
                  "force"      => \$force,
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

#determine distro
if ( `cat /etc/redhat-release 2>/dev/null` =~ "Red Hat" ) {$redhat=1;}

if (! $force) {print "sorry, not supported yet\n"; exit 0;}

#check sudo permissions
if (`id -u` != 0)
{
  print "Run with sudo permissions\n";
  exit -2;
}

#-------------------------------------------------------------------------------
# Make stdout autoflush
#-------------------------------------------------------------------------------
select(STDOUT);
$| = 1;

#-------------------------------------------------------------------------------
# list Devices
#-------------------------------------------------------------------------------
my $remove=`ls -d /sys/devices/pci*/*/$target/pci*/*/remove 2>/dev/null`;
my $reset=`ls -d /sys/devices/pci*/*/$target/reset 2>/dev/null`;
my $rescan=`ls -d /sys/devices/pci*/*/$target/pci*/pci_bus/*/rescan 2>/dev/null`;

chomp $remove;
chomp $reset;
chomp $rescan;
if (! -f $remove)
{
  print "bad target: $target\n";
  exit -4;
}

runcmd("echo 1 > $remove");
runcmd("echo 1 > $reset");

#wait for image_loaded
sleep 2;
my $timer=30;
$rc=-9;

while ($timer--)
{
  print ".";
  sleep 1;
  if (-e $rescan)
  {
    runcmd("echo 1 > $rescan");
    $rc=0;
    last;
  }
}

if ($rc == -9) {print "timeout waiting for $target\n";}

exit 0;

