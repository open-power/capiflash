#! /usr/bin/perl
# IBM_PROLOG_BEGIN_TAG
# This is an automatically generated prolog.
#
# $Source: src/test/vpd/surelock_vpd2rbf.pl $
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
#
  use Fcntl;
  use Getopt::Long;
##
#
# -------------------------------------------------------------------------------
# Variables
# -------------------------------------------------------------------------------
my @parms;			# Parameters from .csv file
my $p;    			# Current Parameter 
my $pc;				# Current Parameter position
my $dt;				# Data Type
my $dl;				# Data Length
my $comment;			# Current VPD Key Word is a comment
my $serial_num;			# Replacement Serial Number
my $card_identifier;     	# Replacement Card Idetification String
my $filename;			# Source VPD File
my $tempfile;			# Temporary Binary File

my $bv;   			# Binary Value
my $fs;   			# File Size 
my $cs;				# Checksum
my $tcs;			# Temporary Checksum
my $vcs;			# Checksum of VPD information
my $bfs;                        # Binary File Size
my $padb;                       # Number of pad bytes

# -------------------------------------------------------------------------------
# Defaults:
# -------------------------------------------------------------------------------
my $sn = 0;			# Current VPD Key Word is the Serial Number
my $prthelp = 0;		# Print Help Menu
my $comp = 0;			# Complete Flag
my $optOK = 0;                  # Options are OK

# -------------------------------------------------------------------------------
# Parse Options
# -------------------------------------------------------------------------------
  $optOK = GetOptions ( "f|file=s"=>\$filename,
                        "s|sn=s"=>\$serial_num,
                        "i|id=s"=>\$card_identifier,
                        "h|help!",\$prthelp
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
    print "Usage: vpd2rbf.pl [-h | --help] [-f | --file] <CommaSepVal.cvs> \n";
    print "    -f or --file   : Comma Separated Value File Containing The VPD Key Words.\n";
    print "    -s or --sn     : Serial Number for the adapter (SN Key Word MUST be in .csv file. This value replaces the .csv contents.\n";
    print "    -i or --id     : Card Identification String (Default: \"Corsa-CAPI PCIe Adapter\").\n";
    print "    -h or --help   : Help - Print this message. \n";
    print "                                                \n";
    print "    Version 1.1                                 \n";
    die "\n";
  }

  print("\n");

  if ($serial_num) {
    print "Serial Number Provided : $serial_num\n";
  }

  if ($card_identifier) {
    print "Card Identifier Provided : $card_identifier Length = $p\n";
  } else {
    $card_identifier = "Corsa-CAPI PCIe Adapter";
  }

  print "The VPD source file is: $filename\n\n" if $filename;

  print "VPD File ==> $filename\n";
  open(VPD, "<", $filename) or die $!;

  $tempfile = substr($filename,0,index($filename,".")) . ".tempvpd";
  $vpdfile = substr($filename,0,index($filename,".")) . ".vpdrbf";

  print "The VPD binary image is: $vpdfile\n";

  if (-e $tempfile) {
    print("ERROR ==> Temporary File $tempfile exist. Please remove / rename and rerun script.\n");
    exit;
  }

  open(VPDTB, ">",$tempfile) or die $!;
  binmode VPDTB;

  print("\n");
  #-------------------------------------------------------------------------------
  # Phase 0: Read and parse the VPD CSV File. Write out a temp binary image.
  # -------------------------------------------------------------------------------

  while (<VPD>) {
    chomp;
    my @parms = split(/,/);

    $pc = 1; 
    $dt = 0;
    $dl = 0;
    $sn = 0;
    $comment = 0;

    if ($comp == 0) {
      foreach $parm (@parms) {
     
        #-------------------------------------------------------------------------------
        # Skip comments and lines past the end of the VPD information
        #-------------------------------------------------------------------------------
        if ( ((substr($parm,0,2) eq "//") || (substr($parm,1,2) eq "//")) && ($pc == 1) ) { 
          $comment = 1;
          if ( (substr($parm,3,3) eq "END") || (substr($parm,4,3) eq "END") || (substr($parm,2,3) eq "END") ) {
            $comp = 1;
            last;
          } else {
            last;
          }
        }
     
        #-------------------------------------------------------------------------------
        # Parse parameters
        #-------------------------------------------------------------------------------
        # First Parameter (Key Word)
        if ($pc == 1) {
          if (substr($parm,0,1) eq '"') {
            $p = substr($parm,1,2);
          } else {
            $p = $parm;
          }
          print ("| $p | ");
          $bv = pack("a*",$p);
          print (VPDTB "$bv");
          if ($p eq "SN") {
            $sn = 1;
          } else {
            $sn = 0;
          }
        }
     
        # Second Parameter (Length)
        if ($pc == 2) {
          $dl = $parm;
          if (($serial_num) && ($sn == 1)) {
            $dl = length($serial_num);
          }
          $bv = pack("C",$dl);
          print (VPDTB "$bv");
          print ("| $dl | ");
        }
     
        # Third Parameter (Data Type)
        if ($pc == 3) {
            if (substr($parm,0,1) eq '"') {
              $p = substr($parm,1,1);
            } else {
              $p = $parm;
            }
          if ($p eq "A") { $dt = 1; }
	    else { $dt = 0;}
        }
     
        # Fourth Parameter (Key Word Information)
        if ($pc == 4) {
          if ($dt == 1) {
            if (substr($parm,0,1) eq '"') {
              $p = substr($parm,1,length($parm) - 2);
            } else {
              $p = $parm;
            }
            if (($serial_num) && ($sn == 1)) {
              $p = $serial_num;
            }
            print ("| $p | ");
            $bv = pack("a*",$p);
            print (VPDTB "$bv");
          } else {
            if (substr($parm,0,1) eq '"') {
              $parm = substr($parm,1,length($parm)-2);
            }
            $cr = length($parm);
            $cs = $parm;
            while ($cr > 0) {  # moved this to 0 from -1 to get rid of extra field at the back??
              $p = substr($cs,length($cs)-2);
              print ("| $p | ");
              $bv = pack("H*",$p);
              print (VPDTB "$bv");
              $cr-=2;
              $cs = substr($cs,0,length($cs)-2);
            }
          }
        }
     
        $pc++;
      }
    }

    if (($comment == 1) || ($comp == 1)) {
      $comment = 0;
    } else {
     print ("\n");
    }
  }

  #-------------------------------------------------------------------------------
  # Add Checksum Keyword. Checksum value will be added in next phase.
  #-------------------------------------------------------------------------------
  $bv = pack("a*","RV");
  print (VPDTB "$bv");
 
  #$dl = 1;
  #$bv = pack("C",$dl);
  #print (VPDTB "$bv");

  close(VPD);
  close(VPDTB);
  # -------------------------------------------------------------------------------
  # End Phase 0: Temp VPD bin file created.
  # -------------------------------------------------------------------------------

  # -------------------------------------------------------------------------------
  # Phase 1: Create final VPD image with tags and checksums.
  # -------------------------------------------------------------------------------
  $fs = -s $tempfile;

  $cs = 0;
  $vcs = 0;

  $bfs = 0;

  open(VPDTB, "<",$tempfile) or die $!;
  binmode VPDTB;
  open(VPDB, ">",$vpdfile) or die $!;
  binmode VPDB;

  # -------------------------------------------------------------------------------
  # Write the Identifier String
  # -------------------------------------------------------------------------------
  $p  = 0x82;
  $bv = pack("C",$p);
  print (VPDB "$bv");
  $cs += unpack("C",$bv);

  $p = length($card_identifier);
  $bv = pack("C",$p);
  print (VPDB "$bv");
  $cs += unpack("C",$bv);
  
  $p  = "00";
  $bv = pack("H*",$p);
  print (VPDB "$bv");
  $cs += unpack("C",$bv);
  
  $p  = $card_identifier;
  for ($i=0; $i<length($p); $i++) {
    $sc = substr($p,$i,1);
    $bv = pack("a",$sc);
    print (VPDB "$bv");
    $cs += unpack("C",$bv);
  }

  $bfs = length($card_identifier) + 3;

  # -------------------------------------------------------------------------------
  # Write the VPD Resource Type
  # -------------------------------------------------------------------------------
  
  $p  = "90";
  $bv = pack("H*",$p);
  print (VPDB "$bv");
  $cs += unpack("C",$bv);

  $padb = (4 - (($bfs + 3 + $fs + 2 + 1) % 4)) % 4;

  $p  = $fs + 2 + $padb; 
  $bv = pack("S",$p);
  print (VPDB "$bv");
  $cs += ($p % 256);
  $cs += (($p>>8) % 256);

  # -------------------------------------------------------------------------------
  # Write the VPD Data
  # -------------------------------------------------------------------------------
  
  while ( read(VPDTB, my $bv, 1) ) {
    print (VPDB "$bv");
     $cs += unpack("C",$bv);
     $vcs += unpack("C",$bv);
  }

  $dl = $padb + 1;
  $bv = pack("C",$dl);
  print (VPDB "$bv");
  $cs += unpack("C",$bv);

  $p  = 256 - ($cs & 255); 
  #$p  = 256 - ($vcs & 255); 
  $bv = pack("C",$p);
  $cs += unpack("C",$bv);
  print (VPDB "$bv");
  $tcs = $p;

  $dl = 0;
  for ($i=0; $i<$padb; $i++) {
    $bv = pack("C",$dl);
    print (VPDB "$bv");
  }

  # -------------------------------------------------------------------------------
  # Write the End Tag
  # -------------------------------------------------------------------------------
  $p  = "78";
  $bv = pack("H*",$p);
  print (VPDB "$bv");
  $cs += unpack("C",$bv);

  close(VPDTB);
  close(VPDB);

  # -------------------------------------------------------------------------------
  # Check the Checksum
  # -------------------------------------------------------------------------------
  open(VPDB, "<",$vpdfile) or die $!;
  binmode VPDB;

  $cs = 0;
  while ( read(VPDB, my $bd, 1) ) {
     $cs += unpack("C",$bd);
  }

  $cs -= 0x78;
  $cs = $cs % 256;
  print ("\n\nThe checksum for $vpdfile : $cs (expected 0)\n");

  close(VPDB);

  system("rm $tempfile");

  exit;
