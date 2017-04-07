IBM Data Engine for NoSQL Software Libraries
============================================

IBM Data Engine for NoSQL - Power Systems Edition creates a new tier of memory by attaching up to 57 Terabytes of auxiliary flash memory to the processor without the latency issues of traditional I/O storage. While not as fast as DRAM, the latency is within the acceptable limit of most applications especially when data is accessed over the network. Flash is also dramatically less expensive than DRAM, and helps reduce the deployment and operational cost for delivering the customer solution. Customers, MSPs, and ISPs all benefit from application of this new technology in the NoSQL application space. Exploiting hardware and software built-in to IBM’s flagship POWER8 open architecture means that clients no longer much choose between “big” or “fast” for their solutions.
Read more, including API guides at:

* [IBM Data Engine for NoSQL Whitepaper](http://ibm.biz/capiflash)

To configure a build environment on Ubuntu:
```
#!/bin/bash
#a ready-to-go toolchain exists in the IBM Toolchain for Linux. Configure its repo and download it
sudo apt-get -y install software-properties-common #needed for the next few lines...
#set up at7.1 repo
wget ftp://ftp.unicamp.br/pub/linuxpatch/toolchain/at/ubuntu/dists/precise/6976a827.gpg.key

sudo apt-key add 6976a827.gpg.key
sudo add-apt-repository "deb ftp://ftp.unicamp.br/pub/linuxpatch/toolchain/at/ubuntu trusty at7.1 "

sudo apt-get -y update
sudo apt-get -y install advance-toolchain-at7.1-runtime advance-toolchain-at7.1-perf advance-toolchain-at7.1-devel advance-toolchain-at7.1-mcore-libs libudev1
```
To configure a build environment on RHEL or Fedora:
```
#!/bin/bash
cat >/etc/yum.repos.d/atX.X.repo
# Beginning of configuration file
[atX.X]
name=Advance Toolchain Unicamp FTP
baseurl=ftp://ftp.unicamp.br/pub/linuxpatch/toolchain/at/redhat/RHEL7
failovermethod=priority
enabled=1
gpgcheck=1
gpgkey=ftp://ftp.unicamp.br/pub/linuxpatch/toolchain/at/redhat/RHEL7/gpg-pubkey-6976a827-5164221b
# End of configuration file

<CTRL-D>
yum install make cscope ctags doxygen git gitk links
yum install advance-toolchain-at7.1-runtime   advance-toolchain-at7.1-devel   advance-toolchain-at7.1-perf  advance-toolchain-at7.1-mcore-libs
```
### API Guide
The IBM Data Engine for NoSQL provides two major sets of public APIs. These are described in:
- [cflash - Block Layer APIs](src/block/README.md)
- [arkdb - Key/Value Layer APIs](src/kv/README.md)


### Building and installing

Builds are configurable for different purposes. If no Data Engine for NoSQL Accelerator is available, you can still do active development using a "File Mode." See below for how to select the mode. Likewise, you may also select the endianness of your code (if needed).

As a developer, to get started:
```
1. clone the repository
2. cd capiflash
3. select a customrc file (see below)
4. source env.bash
5. make cleanall #remove all previous build artifacts
6. make          #build the code, excluding the test code
```
The Test software package relies on Google Test.

Example of acquiring the test framework:
```
pushd src/test/framework
git clone git@github.com:google/googletest.git
popd
OR
apt-get install libgtest-dev
```
Build Targets:
```
make cleanall    #remove all previous build artifacts
make             #build the code, excluding test code
make test        #build only the test code
make buildall    #build the code, including the test code
```

#### customrc - Targeting a specific platform or tuning

Note: Developers have options to enable / disable specific targets (e.g. Big endian PPC64BE vs little endian PPC64EL) or P8 vs P7 tunings. See the customrc.p8be as an example. Creating a new component-specific environment variable is legal, however the env variable should be optional and provide a default that is safe for production.

##### Current valid options

|ENV Variable            | Component | Usage (BOLD = default)|
|----------------------- | --------- | ------------------------|
|TARGET_PLATFORM         | (all)     | PPC64BE - Big-Endian Structures|
|                        |           | PPC64LE - Little-Endian Structures|
|CUSTOMFLAGS             | (all)     | Custom GCC flags. Used typically to enable P8 or P7 tunings, debug, optimization,  etc.|
|BLOCK_FILEMODE_ENABLED  | block     | Forces Block Layer to redirect all IO to a file instead of a CAPI device. 1 = enabled, 0 = disabled|
|BLOCK_KERNEL_MC_ENABLED | block     | Enables block layer to communicate with cxlflash driver built in to the Linux kernel. For more information, see https://www.kernel.org/doc/Documentation/powerpc/cxlflash.txt|

Prebuilt customrc files exist for different environments. Most users will want to use "customrc.p8elblkkermc" or "customrc.p8el:"

##### CustomRC files

|Filename                | Description|
|----------------------- | -------------------------------------|
|customrc.p8el           | Little-endian Linux, P8 Tunings, Block FileMode enabled|
|customrc.p8elblkkermc   | Little-endian Linux, P8 Tunings, Real IO to CXL Flash kernel driver|

#### Utilities
- [Utility reference](src/build/install/resources/README.md)

#### Test Real IO

Example on a POWER8 Little-endian system:
```
#select a /dev/sgX device
/opt/ibm/capikv/bin/cxlfstatus
#setup and run
ln -s customrc.p8elblkkermc customrc
source env.bash
make cleanall
make buildall

ulimit -n 5000; FVT_DEV=/dev/sgX LD_LIBRARY_PATH=.../capiflash/img .../capiflash/obj/tests/run_fvt
```

#### Test File IO

Example on a POWER8 Little-endian system:
  (note that a 1GB file is created in /tmp)
```
ln -s customrc.p8el customrc
source env.bash
make cleanall
make buildall

ulimit -n 5000; LD_LIBRARY_PATH=.../capiflash/img .../capiflash/obj/tests/run_fvt
```
