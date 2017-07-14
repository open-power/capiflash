#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/install/resources/cflash_perst.pl $
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
my $partition0;                                           # reload factory image
my $id="2";
my $prthelp=0;
my $verbose;
my $v="";
my $rc;

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
  print "Usage: cflash_perst.pl [-t <target>]                              \n";
  print "    -t or --target : Target device to perst                       \n";
  print "    -h or --help   : Help - Print this message                    \n";
  print "\n  ex: cflash_perst.pl -t 000N:01:00.0                         \n\n";
  exit 0;
}

#-------------------------------------------------------------------------------
# Parse Options
#-------------------------------------------------------------------------------
if ($#ARGV<0) {usage();}

if (! GetOptions ("t|target=s" => \$target,
                  "p0"         => \$partition0,
                  "v"          => \$verbose,
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

#check sudo permissions
if (`id -u` != 0)
{
  print "Run with sudo permissions to perst\n";
  exit -2;
}

#check cxlflash is not loaded
system("lsmod | grep cxlflash >/dev/null");
if ($? == 0)
{
  print "rmmod cxlflash before perst\n";
  exit -3;
}

#-------------------------------------------------------------------------------
# Make stdout autoflush
#-------------------------------------------------------------------------------
select(STDOUT);
$| = 1;

#-------------------------------------------------------------------------------
# list Devices
#-------------------------------------------------------------------------------
my $basedir=`ls -d /sys/devices/*/*/$target/cxl/card* 2>/dev/null`;
my $image="user";

chomp $basedir;
if (! -d $basedir)
{
  print "bad target: $target\n";
  exit -4;
}
my $cardN=substr $basedir,-1;

if ($partition0)
{
  $image="factory";
}

runcmd("echo $image > $basedir/load_image_on_perst");
runcmd("echo 0 > $basedir/perst_reloads_same_image");
runcmd("cp /usr/share/cxlflash/blacklist-cxlflash.conf /etc/modprobe.d");
runcmd("echo 1 > $basedir/reset");

#wait for image_loaded
my $timer=30;
my $image_loaded;
my $cxlfile="/dev/cxl/afu$cardN.0m";
$rc=-9;

while ($timer--)
{
  print ".";
  sleep 1;
  if (-e $cxlfile)
  {
    $basedir=`ls -d /sys/devices/*/*/$target/cxl/card* 2>/dev/null`;
    chomp $basedir;
    if (-e "$basedir/image_loaded")
    {
      $image_loaded=`cat $basedir/image_loaded`;
      chomp $image_loaded;
      if ($image_loaded ne $image)
      {
        print "image requested was $image, but image_loaded is $image_loaded\n";
        $rc=-5;
        last;
      }
      print "$target successfully loaded $image\n";
      sleep 1;
      $rc=0;
      last;
    }
  }
}

if ($rc == -9) {print "timeout waiting for $target $cxlfile\n";}
else           {runcmd("echo 1 > $basedir/perst_reloads_same_image");}
runcmd("rm /etc/modprobe.d/blacklist-cxlflash.conf");

exit 0;

