#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/build/install/resources/capi_flash.pl $
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
use Fcntl;
use Getopt::Long;

# -------------------------------------------------------------------------------
# Variables
# -------------------------------------------------------------------------------
my $a;                   # Flash Address
my $ra;                  # Flash Read Address
my $dat;                 # Data
my $d;                   # Packed data
my $edat;                # Expected Data
my $fdat;                # Flash Data
my $cfgadr;              # Configuration Register Address
my $rc;                  # Return Code from PSL when performing an operation
my $bc;                  # Block Counter
my $dif;                 # Return code from file read
my $cp;                  # Continue to poll flag
my $st;                  # Start Time
my $lt;                  # Last Poll Time
my $ct;                  # Current Time

my $dif;                 # Data in file flag (End of File)
my $i;                   # Loop Counter

my $et;                  # Total Elasped Time

my $eet;                 # Total/End Erase Time
my $set;                 # Start Erase Time

my $ept;                 # Total Program Time
my $spt;                 # Start Program Time
my $ept;                 # End Program Time

my $evt;                 # Total Verify Time
my $svt;                 # Start Verify Time
my $evt;                 # End Verify Time

my $fsize;               # File size
my $n;                   # Number of Blocks to Program
my $numdev;              # Number of CORSA Devices in the lspci output
my $devstr;              # Device String from lspci output
my @dsev;                # Device String split apart to find device location

# -------------------------------------------------------------------------------
# Defaults:
# -------------------------------------------------------------------------------
my $prthelp = 0;                         # Print Help Flag
my $partition0 = 0;                      # Partition 0 Flag
my $partition1 = 1;                      # Partition 1 Flag
my $partition = "User Partition";        # Programming Partition Name
my $list = "0";
my $optOK = 0;                           # Options are OK
my $filename = "";                       # RBF File Name
my $target  = "";                        # pcie target
my $cfgfile = "";                        # RBF File Name

my $ec = 0;                	         # Error Code

my $ADDR_REG = 0x920;                    # Flash Address Configuration Register
my $SIZE_REG = 0x924;                    # Flash Size Configuration Register
my $CNTL_REG = 0x928;                    # Flash Control / Status Configuration Register
my $DATA_REG = 0x92C;                    # Flash Data Configuration Register

# -------------------------------------------------------------------------------
# Parse Options
# -------------------------------------------------------------------------------
$optOK = GetOptions ( "f|rbf=s"   => \$filename,
                      "p0"        ,  \$partition0,
                      "p1"        ,  \$partition1,
                      "v"         ,  \$vpd,
                      "l"        ,  \$list,
                      "t|target=s" => \$target,
                      "h|help!"   ,  \$prthelp
                    );

if ($ARGV[0]) {
  print "\nUnknown Command Line Options:\n";
  foreach(@ARGV) {
    print "   $_\n";
  }
  print "\n";
  $prthelp = 1;
}

if (!($optOK) | $prthelp) {
  print "\n";
  print "Usage: capi_flash_pgm [-h | --help] [-p0 | -p1 | -v] [-f | --rbf] <RawBinaryBitstream.rbf> \n";
  print "    -p0            : Program Partition 0 - Factory Image          \n";
  print "    -p1            : Program Partition 1 - User Image {Default}   \n";
  print "    -v             : Program VPD Information                      \n";
  print "    -f or --rbf    : Raw Binary File Containing the Bitstream     \n";
  print "    -l             : List pci location of capi devices            \n";
  print "    -t or --target : Target pcie device to flash                  \n";
  print "    -h or --help   : Help - Print this message                    \n";
  die "\n";
}

if ( !(($partition == 0) | ($partition == 1)) ) {
  print "\nPartition can only be 0 or 1\n";
  die;
}


#added to support programming VPD - Charlie Johns
if ( $vpd == 1 ) {
  #note we override the "address" set by the user above, so we cannot program VP
  #simultaneously with another flash region (factory or user).
  $a = 0x1FF0000
} elsif ($partition0) {
  #if we are not programming VPD, see if we should program the "partition 0" image
  $a = 0x10000;
  $partition = "Factory Image"
} else {
  #default to programming partition 1 otherwise.
  $a = 0x850000;
}




# -------------------------------------------------------------------------------
# Open Bitstream File
# -------------------------------------------------------------------------------
if (-e $filename) {
  open(IN, "< $filename");
  binmode(IN);
} elsif (!$list){
  die "Raw Binary Bitstream File $filename does not exist.\n";
}

# -------------------------------------------------------------------------------
# Make stdout autoflush
# -------------------------------------------------------------------------------
select(STDOUT); # default
$| = 1;

if ($list) {

    my @files = </sys/bus/pci/devices/*>;
    my ($vendor, $device);

    $numdev = 0;
    for my $file ( @files ) {
      open(F, $file . "/vendor") or die "Can't open $filename: $!";
      read(F, $vendor, 6);
      close (F);
      open(F, $file . "/device") or die "Can't open $filename: $!";
      read(F, $device, 6);
      close (F);

      if (($vendor eq "0x1014") && (($device eq "0x0477") || ($device eq "0x04cf"))) {
        $cfgfile = $file;
        print "Found CAPI Adapter : $cfgfile\n";
        $numdev++;
      }
    }
exit;
}
# -------------------------------------------------------------------------------
# Find the CAPI Device
# -------------------------------------------------------------------------------
my @files = </sys/bus/pci/devices/*>;
my ($vendor, $device);

if ($target){
   print "Target specified: $target\n";
  $cfgfile = $target;
}
else{
$numdev = 0;
for my $file ( @files ) {
  open(F, $file . "/vendor") or die "Can't open $filename: $!";
  read(F, $vendor, 6);
  close (F);
  open(F, $file . "/device") or die "Can't open $filename: $!";
  read(F, $device, 6);
  close (F);

  if (($vendor eq "0x1014") && (($device eq "0x0477") || ($device eq "0x04cf"))) {
    $cfgfile = $file;
    $numdev++;
  }
 }
}
      
if ($numdev == 0 and $cfgfile eq "") {
  die "CAPI Device (ID = 0x0477) does not exist.\n";
}

if ($numdev > 1) {
   die "\n $numdev capi devices detected";
}

print "\nCAPI Adapter is : $cfgfile\n";

# -------------------------------------------------------------------------------
# Open the CAPI Device's Configuration Space
# -------------------------------------------------------------------------------
sysopen(CFG, $cfgfile . "/config", O_RDWR) or die $!;

# -------------------------------------------------------------------------------
# Read the Device/Vendor ID from the Configuration Space
# -------------------------------------------------------------------------------
sysseek(CFG, 0, SEEK_SET);
sysread(CFG,$dat,4);
$d = unpack("V",$dat);
  
printf("  Device/Vendor ID: 0x%08x\n\n", $d);

# -------------------------------------------------------------------------------
# Read the VSEC Length / VSEC ID from the Configuration Space
# -------------------------------------------------------------------------------
sysseek(CFG, 0x904, SEEK_SET);
sysread(CFG,$dat,4);
$d = unpack("V",$dat);
  
printf("  VSEC Length/VSEC Rev/VSEC ID: 0x%08x\n", $d);
if ( ($d & 0x08000000) == 0x08000000 ) {
  printf("    Version 0.12\n\n");
  $ADDR_REG = 0x950;                    # Flash Address Configuration Register
  $SIZE_REG = 0x954;                    # Flash Size Configuration Register
  $CNTL_REG = 0x958;                    # Flash Control / Status Configuration Register
  $DATA_REG = 0x95C;                    # Flash Data Configuration Register
} else {
  printf("    Version 0.10\n\n");
}

# -------------------------------------------------------------------------------
# Reset Any Previously Aborted Sequences
# -------------------------------------------------------------------------------
$cfgadr = $CNTL_REG;
$dat = 0;
$d = pack("V",$dat);
sysseek CFG, $cfgadr, seek_set;
syswrite(CFG,$d,4);

# -------------------------------------------------------------------------------
# Wait for Flash to be Ready
# -------------------------------------------------------------------------------
sysseek CFG, $cfgadr, seek_set;
sysread(CFG,$d,4);
$dat = unpack("V",$d);

$st = $lt = time();
$cp = 1;

$cfgadr = $CNTL_REG;
while ($cp == 1) {
  sysseek(CFG, $cfgadr, SEEK_SET);
  sysread(CFG,$d,4);
  $rc = unpack("V",$d);
  if ( ($rc & 0x80000000) == 0x80000000 ) {
    $cp = 0;
  }
  $ct = time();
  if (($ct - $lt) > 5) {
    print ".";
    $lt = $ct;
  }
  if (($ct - $st) > 120) {
    print "\nFAILURE --> Flash not ready after 2 min\n";
    $cp = 0;
    $ec = 1;
  }
}

# -------------------------------------------------------------------------------
# Calculate the number of blocks to write
# -------------------------------------------------------------------------------
$fsize = -s $filename;

$n = $fsize / (64 * 1024 * 4);
$n = (($n == int($n)) ? $n : int($n + 1));

printf("Programming %s with %s\n",$partition, $filename);
printf("  Program -> Address: 0x%x for Size: %d in blocks (32K Words or 128K Bytes)\n\n",$a,$n);

$n -= 1;

$set = time();

if ($ec == 0) {
# -------------------------------------------------------------------------------
# Setup for Program From Flash
# -------------------------------------------------------------------------------
  $cfgadr = $ADDR_REG;
  $d = pack("V",$a);
  sysseek CFG, $cfgadr, SEEK_SET;
  syswrite(CFG,$d,4);
  
  $cfgadr = $SIZE_REG;
  $d = pack("V",$n);
  sysseek CFG, $cfgadr, SEEK_SET;
  syswrite(CFG,$d,4);

  $cfgadr = $CNTL_REG;
  $dat = 0x04000000;
  $d = pack("V",$dat);
  sysseek CFG, $cfgadr, SEEK_SET;
  syswrite(CFG,$d,4);

  print "Erasing Flash\n";

# -------------------------------------------------------------------------------
# Wait for Flash Erase to complete.
# -------------------------------------------------------------------------------
  $st = $lt = time();
  $cp = 1;

  $cfgadr = $CNTL_REG;
  while ($cp == 1) {
    sysseek(CFG, $cfgadr, SEEK_SET);
    sysread(CFG,$d,4);
    $rc = unpack("V",$d);
    if ( (($rc & 0x00008000) == 0x00000000) &&
         (($rc & 0x00004000) == 0x00004000) ) {
      $cp = 0;
    }
    $ct = time();
    if (($ct - $lt) > 5) {
      print ".";
      $lt = $ct;
    }
    if (($ct - $st) > 240) {
      print "\nFAILURE --> Erase did not complete in 4 min\n";
      $cp = 0;
      $ec += 2;
    }
  }
}

$eet = $spt = time();

# -------------------------------------------------------------------------------
# Program Flash                        
# -------------------------------------------------------------------------------
if ($ec == 0) {
  print "\n\nProgramming Flash\n";

  $bc = 0;
  print "Writing Block: $bc        \r";
  $cfgadr = $DATA_REG;
  for($i=0; $i<(64*1024*($n+1)); $i++) {
    $dif = read(IN,$d,4);
    $dat = unpack("V",$d);
    if (!($dif)) {
      $dat = 0xFFFFFFFF;
    }
    $d = pack("V",$dat);
    sysseek CFG, $cfgadr, SEEK_SET;
    syswrite(CFG,$d,4);

    if ((($i+1) % (512)) == 0) {
      print "Writing Buffer: $bc        \r";
      $bc++;
    }
  }
}

print "\n\n";

# -------------------------------------------------------------------------------
# Wait for Flash Program to complete.
# -------------------------------------------------------------------------------
$st = $lt = time();
$cp = 1;

$cfgadr = $CNTL_REG;
while ($cp == 1) {
  sysseek(CFG, $cfgadr, SEEK_SET);
  sysread(CFG,$d,4);
  $rc = unpack("V",$d);
  if ( ($rc & 0x40000000) == 0x40000000 ) {
    $cp = 0;
  }
  $ct = time();
  if (($ct - $lt) > 5) {
    print ".";
    $lt = $ct;
  }
  if (($ct - $st) > 120) {
    print "\nFAILURE --> Programming did not complete after 2 min\n";
    $cp = 0;
    $ec += 4;
  }
}

$ept = time();

# -------------------------------------------------------------------------------
# Reset Program Sequence
# -------------------------------------------------------------------------------
$cfgadr = $CNTL_REG;
$dat = 0;
$d = pack("V",$dat);
sysseek CFG, $cfgadr, seek_set;
syswrite(CFG,$d,4);

# -------------------------------------------------------------------------------
# Wait for Flash to be Ready
# -------------------------------------------------------------------------------
sysseek CFG, $cfgadr, seek_set;
sysread(CFG,$d,4);
$dat = unpack("V",$d);

$st = $lt = time();
$cp = 1;

$cfgadr = $CNTL_REG;
while ($cp == 1) {
  sysseek(CFG, $cfgadr, SEEK_SET);
  sysread(CFG,$d,4);
  $rc = unpack("V",$d);
  if ( ($rc & 0x80000000) == 0x80000000 ) {
    $cp = 0;
  }
  $ct = time();
  if (($ct - $lt) > 5) {
    print ".";
    $lt = $ct;
  }
  if (($ct - $st) > 120) {
    print "\nFAILURE --> Flash not ready after 2 min\n";
    $cp = 0;
    $ec += 8;
  }
}

$svt = time();

# -------------------------------------------------------------------------------
# Verify Flash Programmming
# -------------------------------------------------------------------------------
if ($ec == 0) {
  print "Verifying Flash\n";

  seek IN, 0, SEEK_SET;   # Reset to beginning of file
  #close(IN);
  #open(IN, "< $filename");
  #binmode(IN);

  $bc = 0;
  $ra = $a;
  print "Reading Block: $bc        \r";
  for($i=0; $i<(64*1024*($n+1)); $i++) {

    $dif = read(IN,$d,4);
    $edat = unpack("V",$d);
    if (!($dif)) {
      $edat = 0xFFFFFFFF;
    }

    if (($i % 512) == 0) {
      $cfgadr = $CNTL_REG;
      $dat = 0;
      $d = pack("V",$dat);
      sysseek CFG, $cfgadr, seek_set;
      syswrite(CFG,$d,4);

      # -------------------------------------------------------------------------------
      # Wait for Flash to be Ready
      # -------------------------------------------------------------------------------
      $st = $lt = time();
      $cp = 1;
    
      $cfgadr = $CNTL_REG;
      while ($cp == 1) {
        sysseek(CFG, $cfgadr, SEEK_SET);
        sysread(CFG,$d,4);
        $rc = unpack("V",$d);
        if ( ($rc & 0x80000000) == 0x80000000 ) {
          $cp = 0;
        }
        $ct = time();
        if (($ct - $lt) > 5) {
          print ".";
          $lt = $ct;
        }
        if (($ct - $st) > 120) {
          print "\nFAILURE --> Flash not ready after 2 min\n";
          $cp = 0;
          $ec += 16;
          last;
        }
      }

      # -------------------------------------------------------------------------------
      # Setup for Reading From Flash
      # -------------------------------------------------------------------------------
      $cfgadr = $ADDR_REG;
      $d = pack("V",$ra);
      sysseek CFG, $cfgadr, SEEK_SET;
      syswrite(CFG,$d,4);
      $ra += 0x200;
    
      $cfgadr = $SIZE_REG;
      $d = pack("V",0x1FF);
      sysseek CFG, $cfgadr, SEEK_SET;
      syswrite(CFG,$d,4);
    
      $cfgadr = $CNTL_REG;
      $dat = 0x08000000;
      $d = pack("V",$dat);
      sysseek CFG, $cfgadr, SEEK_SET;
      syswrite(CFG,$d,4);
    }

    $cfgadr = $DATA_REG;
    sysseek CFG, $cfgadr, SEEK_SET;
    sysread(CFG,$d,4);
    $fdat = unpack("V",$d);

    if ($edat != $fdat) {
      $ma = $ra + ($i % 512) - 0x200;
      printf("Data Miscompare @: %08x --> %08x expected %08x\r",$ma, $fdat, $edat);
      $rc = <STDIN>;
    }

    if ((($i+1) % (64*1024)) == 0) {
      print "Reading Block: $bc        \r";
      $bc++;
    }
  }
}

$evt = time();

print "\n\n";

# -------------------------------------------------------------------------------
# Check for Errors during Programming
# -------------------------------------------------------------------------------
if ($ec != 0) {
  print "\nErrors Occured : Error Code => $ec\n";
}

# -------------------------------------------------------------------------------
# Calculate and Print Elapsed Times
# -------------------------------------------------------------------------------
$et = $evt - $set;
$eet = $eet - $set;
$ept = $ept - $spt;
$evt = $evt - $svt;

print "Erase Time:   $eet seconds\n";
print "Program Time: $ept seconds\n";
print "Verify Time:  $evt seconds\n";
print "Total Time:   $et seconds\n\n";

# -------------------------------------------------------------------------------
# Reset Read Sequence
# -------------------------------------------------------------------------------
$cfgadr = $CNTL_REG;
$dat = 0;
$d = pack("V",$dat);
sysseek CFG, $cfgadr, seek_set;
syswrite(CFG,$d,4);

close(IN);
close(CFG);
exit;
