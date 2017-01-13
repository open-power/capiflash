#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/install/resources/cflash_devices.pl $
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
my $type="0601";
my $prthelp=0;
my $single;
my $verbose;

sub usage()
{
  print "\n";
  print "Usage: cflash_devices.pl [-t type] [-s] [-v]                  \n";
  print "    -t or --type   : Type to list                             \n";
  print "    -s or --single : Output is a single line                  \n";
  print "    -v             : verbose for debug                        \n";
  print "    -h or --help   : Help - Print this message                \n";
  print "\n  ex: cflash_devices.pl -t 0601                           \n\n";
  exit 0;
}

#-------------------------------------------------------------------------------
# Parse Options
#-------------------------------------------------------------------------------
if (! GetOptions ("t|type=s"   => \$type,
                  "s"          => \$single,
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

#-------------------------------------------------------------------------------
# Make stdout autoflush
#-------------------------------------------------------------------------------
select(STDOUT);
$| = 1;

#-------------------------------------------------------------------------------
# get wwids matching $type
#-------------------------------------------------------------------------------
my @cards = `lspci |grep $type`;
my $cmd="";
my @list;
my @ids;
my $sline="";
my @devs;
my $dev;
my $wwid;
my %lookup;
my @wwids;
my @sorted;

for my $adap (@cards)
{
  my @Acard=split / /, $adap;
  my $card=$Acard[0];
  chomp $card;
  my @D=`ls -d /sys/devices/*/*/$card/*/*/host*/target*:*:*/*:*:*:*/scsi_generic/*|awk -F/ '{print \$13}' 2>/dev/null`;
  for $dev (@D)
  {
    chomp $dev;
    push(@devs, $dev);
    if ($verbose) {print "(" . join(" ", @devs) . ")\n";}
  }
}

for $dev (@devs)
{
  my $tmp=`/lib/udev/scsi_id -d /dev/$dev --page=0x83 --whitelisted`;
  $wwid=substr $tmp,1;
  chomp $wwid;
  #only use devs in superpipe mode
  my $out=`grep $wwid /opt/ibm/capikv/etc/sioluntable.ini|grep =1`;
  chomp $out;
  $verbose && print "$dev $out\n";
  push(@list,$wwid);
  $lookup{$dev} = $wwid;
}

@wwids = sort @list;

$verbose && print "Sorted by wwid:\n";

foreach $wwid (@wwids)
{
  foreach my $key ( keys %lookup )
  {
    if ($lookup{$key} =~ /$wwid/)
    {
      $verbose && print "$key=>$lookup{$key}\n";
      push(@sorted,$key);
      $lookup{$key} = "empty";
      last;
    }
  }
}

if (! @sorted) {exit 0;}

if ($single) {my $out="/dev/" . join(":/dev/",@sorted); print "$out\n"; exit 0;}

for $dev (@sorted) {print "/dev/$dev\n";}

exit 0;

