#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/test/pticks.pl $
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
use Time::HiRes qw(usleep);

#foreach (split(//, $d1)) {
#    printf("%02x", ord($_));
#}
#print "\n";

my $running=1;
my @uticks;
my @sticks;
my $STAT;
my $ut=0;
my $st=0;
my $i=0;

#input string of process ids, output sum of ticks for all pids

sub get_linux
{
  while ( $running==1 )
  {
    $running=0;

    for ($i=0; $i<@ARGV; $i++)
    {
      my $open=0;

      my $file = "/proc/$ARGV[$i]/stat";

      open(STAT, $file) or next;

      $running=1;

      seek (STAT, SEEK_SET, 0);

      my @stat = split /\s+/ , <STAT>;
      $uticks[$i] = $stat[13];
      $sticks[$i] = $stat[14];
      close(STAT);
    }
    usleep(100000);
  }
}

sub get_aix
{
  my $pdata=0;
  my $nb=0;

  while ( $running==1 )
  {
    $running=0;

    for (my $i=0; $i<@ARGV; $i++)
    {
      my $open=0;

      open my $fh, '<:raw', "/proc/$ARGV[$i]/status" or next;

      seek $fh, 124, 0;
      $nb  = read $fh, $pdata, 48;
      close $fh;

      my ($utv_sec, $utv_nsec, undef, undef, $stv_sec, $stv_nsec) = unpack 'N N N N N N', $pdata;

      $uticks[$i] = ($utv_sec*100) + ($utv_nsec/10000000);
      $sticks[$i] = ($stv_sec*100) + ($stv_nsec/10000000);
#      printf("pid:%d utime:%d stime:%d\n", $ARGV[$i], $uticks[$i, $sticks[$i]);

      $running=1;
    }
    usleep(1000000);
  }
}

#MAIN
`uname -a|grep AIX`;
if ($? == 0) {get_aix();}
else         {get_linux();}

for ($i=0; $i<@ARGV; $i++)
{
  if (defined($uticks[$i])) {print "$ARGV[$i] : sys: $sticks[$i] usr:$uticks[$i]\n"; $ut+=$uticks[$i];}
  if (defined($sticks[$i])) {$st+=$sticks[$i];}
}

printf "uticks:%d sticks:%d\n", $ut, $st;
exit 0;

