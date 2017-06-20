IBM Data Engine for NoSQL Software Libraries
============================================

IBM Data Engine for NoSQL - Power Systems Edition creates a new tier of memory by attaching up to 57 Terabytes of auxiliary flash memory to the processor without the latency issues of traditional I/O storage. While not as fast as DRAM, the latency is within the acceptable limit of most applications especially when data is accessed over the network. Flash is also dramatically less expensive than DRAM, and helps reduce the deployment and operational cost for delivering the customer solution. Customers, MSPs, and ISPs all benefit from the application of this new technology in the NoSQL application space. Exploiting hardware and software built-in to IBM’s flagship POWER8 open architecture means that clients no longer must choose between “big” or “fast” for their solutions.
Read more, including API guides at:

* [IBM Data Engine for NoSQL Whitepaper](http://ibm.biz/capiflash)


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
5. src/build/install/resources/gtest_add.sh  #insert the Google Test framework
6. make configure
7. make cleanall
8. make install
```
Build Targets that set rpath=/opt/ibm/capikv/lib
```
make prod        #build the code, excluding test code
make prodall     #build all the code
make install     #build all the code, and install a development setup
```
Build Targets that do not set rpath (use LD_LIBRARY_PATH=.../capiflash/img or LD_LIBRARY_PATH=/opt/ibm/capikv/lib)
```
make             #build the code, excluding test code
make test        #build only the test code
make buildall    #build all the code
```
*if all the code is built, then use "make installsb" to install your build objects to /opt/ibm/capikv/*.

#### Targeting a specific platform or tuning - the "customrc" file 

Note: Developers have options to enable / disable specific targets (e.g. Big endian PPC64BE vs little endian PPC64EL) or P8 vs P7 tunings. See the customrc.p8be as an example. Creating a new component-specific environment variable is legal, however the env variable should be optional and provide a default that is safe for production.

##### Current valid options

|ENV Variable            | Component | Usage (BOLD = default)|
|----------------------- | --------- | ------------------------|
|TARGET_PLATFORM         | (all)     | PPC64BE - Big-Endian Structures|
|                        |           | PPC64LE - Little-Endian Structures|
|CUSTOMFLAGS             | (all)     | Custom GCC flags. Used typically to enable P8 or P7 tunings, debug, optimization,  etc.|
|BLOCK_FILEMODE_ENABLED  | block     | Forces Block Layer to redirect all IO to a file instead of a CAPI device. 1 = enabled, 0 = disabled|
|BLOCK_KERNEL_MC_ENABLED | block     | Enables block layer to communicate with cxlflash driver built in to the Linux kernel. For more information, see https://www.kernel.org/doc/Documentation/powerpc/cxlflash.txt|

Prebuilt customrc files exist for different environments. Most users will want to use "customrc.p8elblkkermc" or "customrc.p8el"

|Filename                | Description|
|----------------------- | -------------------------------------|
|customrc.p8el           | Little-endian Linux, P8 Tunings, Block FileMode enabled|
|customrc.p8elblkkermc   | Little-endian Linux, P8 Tunings, Real IO to CXL Flash kernel driver|

#### Utilities
- [Utility reference](src/build/install/resources/README.md)

#### Test File IO

Example on a POWER8 Little-endian system:
```
#build
cd .../capiflash
ln -s customrc.p8el customrc
source env.bash
make cleanall
make install
#create a 4gb test file in /tmp
fallocate -l 4g /tmp/testfile
#run the FVT
FVT_DEV=/tmp/testfile /opt/ibm/capikv/test/run_fvt
#run FILE IO
/opt/ibm/capikv/bin/blockio -d /tmp/testfile
```

#### Test Real IO

Example on a POWER8 Little-endian system:
```
#build
cd .../capiflash
ln -s customrc.p8elblkkermc customrc
source env.bash
make cleanall
make install
#select a /dev/sgX device
sudo /opt/ibm/capikv/bin/cxlfrefreshluns
sudo /opt/ibm/capikv/bin/cxlfstatus
  sg8:     1:0:0:0,   sdc,     legacy,    60025380025382462300058000000000
#set the lunmode to superpipe
sudo /opt/ibm/capikv/bin/cxlfsetlunmode 60025380025382462300058000000000 1
#run the FVT
sudo FVT_DEV=/dev/sg8 /opt/ibm/capikv/test/run_fvt
#run Real IO
sudo /opt/ibm/capikv/bin/blockio -d /dev/sg8 -p
```
