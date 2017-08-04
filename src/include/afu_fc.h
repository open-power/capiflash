/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/afu_fc.h $                                        */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2015                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#ifndef AFU_FC_H
#define AFU_FC_H


/**
 * @enum AFU_PORT_ID
 * @brief Typedef of the two supported error log levels.
 * Informational logs will be hidden from the user. Predictive logs will
 * be reported. All predictive errors MUST have a callout.
 * MAX_WWPNS_PER_ADAPTER must be the last item in the list!
 */
typedef enum
{
    AFU_PORT_ID_TOP       = 0x00,
    AFU_PORT_ID_BOTTOM    = 0x01,
    MAX_WWPNS_PER_ADAPTER // This MUST be last
} AFU_PORT_ID;


// FC module register address offset for each port (byte address)
#define FC_PORT_REG_SIZE  0x1000
#define FC_PORT_BASE_OFFSET  0x2012000
#define FC_PORT_MMAP_SIZE ((FC_PORT_BASE_OFFSET) + (MAX_WWPNS_PER_ADAPTER) * (FC_PORT_REG_SIZE))

#define SET_BASE(_p,_b)                                                               \
do                                                                                    \
{                                                                                     \
  if (_p<2) {_b = FC_PORT_BASE_OFFSET + (_p*FC_PORT_REG_SIZE);}                       \
  else      {_b = FC_PORT_BASE_OFFSET + (2*FC_PORT_REG_SIZE) + (_p*FC_PORT_REG_SIZE);}\
} while (0);


// FC module register offset (byte address)
#define FC_MTIP_REV 0x000
#define FC_MTIP_SCRATCH 0x008
#define FC_MTIP_CMDCONFIG 0x010
#define FC_MTIP_STATUS 0x018
#define FC_MTIP_INITTIMER 0x020
#define FC_MTIP_EVENTTIME 0x028
#define FC_MTIP_CREDIT 0x030
#define FC_MTIP_BB_SCN 0x038
#define FC_MTIP_RX_SF 0x040
#define FC_MTIP_TX_SE 0x048
#define FC_MTIP_TX_SF 0x050
#define FC_MTIP_RX_AE 0x058
#define FC_MTIP_RX_AF 0x060
#define FC_MTIP_TX_AE 0x068
#define FC_MTIP_TX_AF 0x070
#define FC_MTIP_FRMLEN 0x078
#define FC_MTIP_SD_RCFG_CMD 0x100
#define FC_MTIP_SD_RCFG_WRDAT 0x108
#define FC_MTIP_SD_RCFG_RDDAT 0x110
#define FC_MTIP_TX_FRM_CNT 0x200
#define FC_MTIP_TX_CRC_ERR_CNT 0x208
#define FC_MTIP_RX_FRM_CNT 0x210
#define FC_MTIP_RX_CRC_ERR_CNT 0x218
#define FC_MTIP_RX_LGTH_ERR_CNT 0x220
#define FC_MTIP_FRM_DISC_CNT 0x228

#define FC_PNAME 0x300
#define FC_NNAME 0x308
#define FC_PORT_ID 0x310
#define FC_CONFIG 0x320
#define FC_CONFIG2 0x328
#define FC_STATUS 0x330
#define FC_TIMER 0x338
#define FC_E_D_TOV 0x340
#define FC_ERROR 0x380
#define FC_ERRCAP 0x388
#define FC_ERRMSK 0x390
#define FC_ERRINJ 0x3A0
#define FC_TGT_D_ID 0x400
#define FC_TGT_PNAME 0x408
#define FC_TGT_NNAME 0x410
#define FC_TGT_LOGI 0x418
#define FC_TGT_B2BCR 0x420
#define FC_TGT_E_D_TOV 0x428
#define FC_TGT_CLASS3 0x430
#define FC_CNT_CLKRATIO 0x500
#define FC_CNT_FCREFCLK 0x508
#define FC_CNT_PCICLK 0x510
#define FC_CNT_TXRDWR 0x518
#define FC_CNT_TXLOGI 0x520
#define FC_CNT_TXDATA 0x528
#define FC_CNT_LINKERR 0x530
#define FC_CNT_CRCERR 0x538
#define FC_CNT_CRCERR_RO 0x540
#define FC_CNT_OTHERERR 0x548
#define FC_CNT_TIMEOUT 0x550
#define FC_CRC_THRESH 0x580
#define FC_DBGDISP 0x600
#define FC_DBGDATA 0x608
#define FC_LOOPBACK_TXCNT 0x670
#define FC_LOOPBACK_PASSCNT 0x678
#define FC_LOOPBACK_ERRCNT 0x680

#define FC_MTIP_CMDCONFIG_ONLINE    0x20ull
#define FC_MTIP_CMDCONFIG_OFFLINE   0x40ull

#define FC_MTIP_STATUS_MASK         0x30ull
#define FC_MTIP_STATUS_ONLINE       0x20ull
#define FC_MTIP_STATUS_OFFLINE      0x10ull

#define FC_CNT_CRCTOT 0x610
#define FC_CNT_AFURD 0x618
#define FC_CNT_AFUWR 0x620
#define FC_CNT_AFUABORT 0x628
#define FC_CNT_RSPOVER 0x630
#define FC_CNT_ABORT1 0x638
#define FC_CNT_ABORT2 0x640
#define FC_CNT_ABORT3 0x648
#define FC_CNT_ABORT4 0x650
#define FC_CNT_WBUFREL 0x658
#define FC_CNT_RXRSP 0x660

// command encodes
// perl -ne 'if( /localparam\s+(\S+)\s*=\s*32.h([0-9a-f]+)/i ) { print "#define $1 0x$2\n";}' fc_module/src/ctms_fc_afu.inc
#define FCP_READ 0x01
#define FCP_WRITE 0x02
#define FCP_GSCSI_RD 0x03
#define FCP_GSCSI_WR 0x04
#define FCP_ABORT 0x05

// response interface encodes
#define FCP_RSP_GOOD 0x00
#define FCP_RSP_CHECK 0x02
#define FCP_RSP_BUSY 0x08
#define FCP_RSP_CRCERR 0x51
#define FCP_RSP_ABORTPEND 0x52
#define FCP_RSP_BADREQ 0x53
#define FCP_RSP_NOLOGI 0x54
#define FCP_RSP_NOEXP 0x55
#define FCP_RSP_INUSE 0x56
#define FCP_RSP_LINKDOWN 0x57
#define FCP_RSP_ABORTOK 0x58
#define FCP_RSP_ABORTFAIL 0x59
#define FCP_RSP_FCPERR 0x60

#endif
