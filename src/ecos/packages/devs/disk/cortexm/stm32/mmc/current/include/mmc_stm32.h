//==========================================================================
//
//      mmc_stm32.h
//
//      MMC driver for STM32 CortexM processors
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2010, 2011 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later
// version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with eCos; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// As a special exception, if other files instantiate templates or use
// macros or inline functions from this file, or you compile this file
// and link it with other works to produce a work based on this file,
// this file does not by itself cause the resulting work to be covered by
// the GNU General Public License. However the source code for this file
// must still be made available in accordance with section (3) of the GNU
// General Public License v2.
//
// This exception does not invalidate any other reasons why a work based
// on this file might be covered by the GNU General Public License.
// -------------------------------------------
// ####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):     reille
// Contributors:
// Date:          2013-06-26
// Description:   MMC driver for STM32 CortexM processors
//####DESCRIPTIONEND####
//==========================================================================

#ifndef CYGONCE_DEVS_DISK_CORTEXM_STM32_MMC_H
#define CYGONCE_DEVS_DISK_CORTEXM_STM32_MMC_H

#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/var_dma.h>



// ----------------------------------------------------------------------------

typedef enum {
/**
  * @brief  SDIO specific error defines
  */
  SD_CMD_CRC_FAIL                    = (1), /*!< Command response received (but CRC check failed) */
  SD_DATA_CRC_FAIL                   = (2), /*!< Data bock sent/received (CRC check Failed) */
  SD_CMD_RSP_TIMEOUT                 = (3), /*!< Command response timeout */
  SD_DATA_TIMEOUT                    = (4), /*!< Data time out */
  SD_TX_UNDERRUN                     = (5), /*!< Transmit FIFO under-run */
  SD_RX_OVERRUN                      = (6), /*!< Receive FIFO over-run */
  SD_START_BIT_ERR                   = (7), /*!< Start bit not detected on all data signals in widE bus mode */
  SD_CMD_OUT_OF_RANGE                = (8), /*!< CMD's argument was out of range.*/
  SD_ADDR_MISALIGNED                 = (9), /*!< Misaligned address */
  SD_BLOCK_LEN_ERR                   = (10), /*!< Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
  SD_ERASE_SEQ_ERR                   = (11), /*!< An error in the sequence of erase command occurs.*/
  SD_BAD_ERASE_PARAM                 = (12), /*!< An Invalid selection for erase groups */
  SD_WRITE_PROT_VIOLATION            = (13), /*!< Attempt to program a write protect block */
  SD_LOCK_UNLOCK_FAILED              = (14), /*!< Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card */
  SD_COM_CRC_FAILED                  = (15), /*!< CRC check of the previous command failed */
  SD_ILLEGAL_CMD                     = (16), /*!< Command is not legal for the card state */
  SD_CARD_ECC_FAILED                 = (17), /*!< Card internal ECC was applied but failed to correct the data */
  SD_CC_ERROR                        = (18), /*!< Internal card controller error */
  SD_GENERAL_UNKNOWN_ERROR           = (19), /*!< General or Unknown error */
  SD_STREAM_READ_UNDERRUN            = (20), /*!< The card could not sustain data transfer in stream read operation. */
  SD_STREAM_WRITE_OVERRUN            = (21), /*!< The card could not sustain data programming in stream mode */
  SD_CID_CSD_OVERWRITE               = (22), /*!< CID/CSD overwrite error */
  SD_WP_ERASE_SKIP                   = (23), /*!< only partial address space was erased */
  SD_CARD_ECC_DISABLED               = (24), /*!< Command has been executed without using internal ECC */
  SD_ERASE_RESET                     = (25), /*!< Erase sequence was cleared before executing because an out of erase sequence command was received */
  SD_AKE_SEQ_ERROR                   = (26), /*!< Error in sequence of authentication. */
  SD_INVALID_VOLTRANGE               = (27),
  SD_ADDR_OUT_OF_RANGE               = (28),
  SD_SWITCH_ERROR                    = (29),
  SD_SDIO_DISABLED                   = (30),
  SD_SDIO_FUNCTION_BUSY              = (31),
  SD_SDIO_FUNCTION_FAILED            = (32),
  SD_SDIO_UNKNOWN_FUNCTION           = (33),

/**
  * @brief  Standard error defines
  */
  SD_INTERNAL_ERROR,
  SD_NOT_CONFIGURED,
  SD_REQUEST_PENDING,
  SD_REQUEST_NOT_APPLICABLE,
  SD_INVALID_PARAMETER,
  SD_UNSUPPORTED_FEATURE,
  SD_UNSUPPORTED_HW,
  SD_ERROR,
  SD_OK
} stm32_sd_error_t;

// ----------------------------------------------------------------------------

#define SD_ALLZERO                      ((cyg_uint32)0x00000000)

#define SD_WIDE_BUS_SUPPORT             ((cyg_uint32)0x00040000)
#define SD_SINGLE_BUS_SUPPORT           ((cyg_uint32)0x00010000)
#define SD_CARD_LOCKED                  ((cyg_uint32)0x02000000)


#define SD_DATATIMEOUT                  ((cyg_uint32)0x000FFFFF)
#define SD_0TO7BITS                     ((cyg_uint32)0x000000FF)
#define SD_8TO15BITS                    ((cyg_uint32)0x0000FF00)
#define SD_16TO23BITS                   ((cyg_uint32)0x00FF0000)
#define SD_24TO31BITS                   ((cyg_uint32)0xFF000000)
#define SD_MAX_DATA_LENGTH              ((cyg_uint32)0x01FFFFFF)


#define SD_DMA_MODE                     ((cyg_uint32)0x00000000)
#define SD_INTERRUPT_MODE               ((cyg_uint32)0x00000001)
#define SD_POLLING_MODE                 ((cyg_uint32)0x00000002)


// ----------------------------------------------------------------------------

/**
  * @brief  Mask for errors Card Status R1 (OCR Register)
  */
#define SD_OCR_ADDR_OUT_OF_RANGE        ((cyg_uint32)0x80000000)
#define SD_OCR_ADDR_MISALIGNED          ((cyg_uint32)0x40000000)
#define SD_OCR_BLOCK_LEN_ERR            ((cyg_uint32)0x20000000)
#define SD_OCR_ERASE_SEQ_ERR            ((cyg_uint32)0x10000000)
#define SD_OCR_BAD_ERASE_PARAM          ((cyg_uint32)0x08000000)
#define SD_OCR_WRITE_PROT_VIOLATION     ((cyg_uint32)0x04000000)
#define SD_OCR_LOCK_UNLOCK_FAILED       ((cyg_uint32)0x01000000)
#define SD_OCR_COM_CRC_FAILED           ((cyg_uint32)0x00800000)
#define SD_OCR_ILLEGAL_CMD              ((cyg_uint32)0x00400000)
#define SD_OCR_CARD_ECC_FAILED          ((cyg_uint32)0x00200000)
#define SD_OCR_CC_ERROR                 ((cyg_uint32)0x00100000)
#define SD_OCR_GENERAL_UNKNOWN_ERROR    ((cyg_uint32)0x00080000)
#define SD_OCR_STREAM_READ_UNDERRUN     ((cyg_uint32)0x00040000)
#define SD_OCR_STREAM_WRITE_OVERRUN     ((cyg_uint32)0x00020000)
#define SD_OCR_CID_CSD_OVERWRIETE       ((cyg_uint32)0x00010000)
#define SD_OCR_WP_ERASE_SKIP            ((cyg_uint32)0x00008000)
#define SD_OCR_CARD_ECC_DISABLED        ((cyg_uint32)0x00004000)
#define SD_OCR_ERASE_RESET              ((cyg_uint32)0x00002000)
#define SD_OCR_AKE_SEQ_ERROR            ((cyg_uint32)0x00000008)
#define SD_OCR_ERRORBITS                ((cyg_uint32)0xFDFFE008)


// ----------------------------------------------------------------------------

/* SDIO Static flags for SDIO STA register */
#define STM32_SDIO_STA_REG_STATIC_FLAGS             ((cyg_uint32)0x000005FF)


/**
  * @brief  SDIO Intialization Frequency (400KHz max)
  */
#define STM32_SDIO_TRANSFER_CLK_DIV_400KHz          ((cyg_uint8)0xB2)
/**
  * @brief  SDIO Data Transfer Frequency (25MHz max)
  * 这个域定义了输入时钟(SDIOCLK)与输出时钟(SDIO_CK)间的分频系数:
  * SDIO_CK频率 = SDIOCLK/[CLKDIV + 2]
  *
  * SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_TRANSFER_CLK_DIV),
  */
#define STM32_SDIO_TRANSFER_CLK_DIV_24MHz           ((cyg_uint8)0x02)// 24MHz
#define STM32_SDIO_TRANSFER_CLK_DIV_18MHz           ((cyg_uint8)0x04)



// ----------------------------------------------------------------------------

/** @defgroup SDIO_Bus_Wide
  * @{
  */

#define SDIO_BusWide_1b                             ((cyg_uint32)0x00000000)
#define SDIO_BusWide_4b                             ((cyg_uint32)0x00000800)
#define SDIO_BusWide_8b                             ((cyg_uint32)0x00001000)

// ----------------------------------------------------------------------------


typedef struct {
    cyg_uint32 argument;  /*!< Specifies the SDIO command argument which is sent
                               to a card as part of a command message. If a command
                               contains an argument, it must be loaded into this register
                               before writing the command to the command register */

    cyg_uint32 cmd_index; /*!< Specifies the SDIO command index. It must be lower than 0x40. */

    cyg_uint32 response;  /*!< Specifies the SDIO response type.
                               This parameter can be a value of @ref SDIO_Response_Type */

    cyg_uint32 wait;      /*!< Specifies whether SDIO wait-for-interrupt request is enabled or disabled.
                               This parameter can be a value of @ref SDIO_Wait_Interrupt_State */

    cyg_uint32 cpsm;      /*!< Specifies whether SDIO Command path state machine (CPSM)
                               is enabled or disabled.
                               This parameter can be a value of @ref SDIO_CPSM_State */
} stm32_sdio_cmd_t;

#define STM32_SDIO_CMD(_name_,_arg,_cmd_index,_response,_wait,_cpsm) \
    stm32_sdio_cmd_t _name_ = {     \
        .argument   = _arg,         \
        .cmd_index  = _cmd_index,   \
        .response   = _response,    \
        .wait       = _wait,        \
        .cpsm       = _cpsm,        \
}


typedef struct {
  cyg_uint32 SDIO_DataTimeOut;    /*!< Specifies the data timeout period in card bus clock periods. */

  cyg_uint32 SDIO_DataLength;     /*!< Specifies the number of data bytes to be transferred. */

  cyg_uint32 SDIO_DataBlockSize;  /*!< Specifies the data block size for block transfer.
                                     This parameter can be a value of @ref SDIO_Data_Block_Size */

  cyg_uint32 SDIO_TransferDir;    /*!< Specifies the data transfer direction, whether the transfer
                                     is a read or write.
                                     This parameter can be a value of @ref SDIO_Transfer_Direction */

  cyg_uint32 SDIO_TransferMode;   /*!< Specifies whether data transfer is in stream or block mode.
                                     This parameter can be a value of @ref SDIO_Transfer_Type */

  cyg_uint32 SDIO_DPSM;           /*!< Specifies whether SDIO Data path state machine (DPSM)
                                     is enabled or disabled.
                                     This parameter can be a value of @ref SDIO_DPSM_State */
} stm32_sdio_data_config_t;





// ----------------------------------------------------------------------------

/**
  * @brief Supported SD Memory Cards
  */
typedef enum {
    SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V1_1 = 0,
    SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V2_0,
    SD_CARD_TYPE_HIGH_CAPACITY_SD_CARD,
    SD_CARD_TYPE_MULTIMEDIA_CARD,
    SD_CARD_TYPE_SECURE_DIGITAL_IO_CARD,
    SD_CARD_TYPE_HIGH_SPEED_MULTIMEDIA_CARD,
    SD_CARD_TYPE_SECURE_DIGITAL_IO_COMBO_CARD,
    SD_CARD_TYPE_HIGH_CAPACITY_MMC_CARD,
} sd_card_type_t;


// ----------------------------------------------------------------------------

/**
  * @brief SDIO Commands  Index
  */
#define SD_CMD_GO_IDLE_STATE                       ((cyg_uint8)0)
#define SD_CMD_SEND_OP_COND                        ((cyg_uint8)1)
#define SD_CMD_ALL_SEND_CID                        ((cyg_uint8)2)
#define SD_CMD_SET_REL_ADDR                        ((cyg_uint8)3) /*!< SDIO_SEND_REL_ADDR for SD Card */
#define SD_CMD_SET_DSR                             ((cyg_uint8)4)
#define SD_CMD_SDIO_SEN_OP_COND                    ((cyg_uint8)5)
#define SD_CMD_HS_SWITCH                           ((cyg_uint8)6)
#define SD_CMD_SEL_DESEL_CARD                      ((cyg_uint8)7)
#define SD_CMD_HS_SEND_EXT_CSD                     ((cyg_uint8)8)
#define SD_CMD_SEND_CSD                            ((cyg_uint8)9)
#define SD_CMD_SEND_CID                            ((cyg_uint8)10)
#define SD_CMD_READ_DAT_UNTIL_STOP                 ((cyg_uint8)11) /*!< SD Card doesn't support it */
#define SD_CMD_STOP_TRANSMISSION                   ((cyg_uint8)12)
#define SD_CMD_SEND_STATUS                         ((cyg_uint8)13)
#define SD_CMD_HS_BUSTEST_READ                     ((cyg_uint8)14)
#define SD_CMD_GO_INACTIVE_STATE                   ((cyg_uint8)15)
#define SD_CMD_SET_BLOCKLEN                        ((cyg_uint8)16)
#define SD_CMD_READ_SINGLE_BLOCK                   ((cyg_uint8)17)
#define SD_CMD_READ_MULT_BLOCK                     ((cyg_uint8)18)
#define SD_CMD_HS_BUSTEST_WRITE                    ((cyg_uint8)19)
#define SD_CMD_WRITE_DAT_UNTIL_STOP                ((cyg_uint8)20) /*!< SD Card doesn't support it */
#define SD_CMD_SET_BLOCK_COUNT                     ((cyg_uint8)23) /*!< SD Card doesn't support it */
#define SD_CMD_WRITE_SINGLE_BLOCK                  ((cyg_uint8)24)
#define SD_CMD_WRITE_MULT_BLOCK                    ((cyg_uint8)25)
#define SD_CMD_PROG_CID                            ((cyg_uint8)26) /*!< reserved for manufacturers */
#define SD_CMD_PROG_CSD                            ((cyg_uint8)27)
#define SD_CMD_SET_WRITE_PROT                      ((cyg_uint8)28)
#define SD_CMD_CLR_WRITE_PROT                      ((cyg_uint8)29)
#define SD_CMD_SEND_WRITE_PROT                     ((cyg_uint8)30)
#define SD_CMD_SD_ERASE_GRP_START                  ((cyg_uint8)32) /*!< To set the address of the first write
                                                                  block to be erased. (For SD card only) */
#define SD_CMD_SD_ERASE_GRP_END                    ((cyg_uint8)33) /*!< To set the address of the last write block of the
                                                                  continuous range to be erased. (For SD card only) */
#define SD_CMD_ERASE_GRP_START                     ((cyg_uint8)35) /*!< To set the address of the first write block to be erased.
                                                                  (For MMC card only spec 3.31) */

#define SD_CMD_ERASE_GRP_END                       ((cyg_uint8)36) /*!< To set the address of the last write block of the
                                                                  continuous range to be erased. (For MMC card only spec 3.31) */

#define SD_CMD_ERASE                               ((cyg_uint8)38)
#define SD_CMD_FAST_IO                             ((cyg_uint8)39) /*!< SD Card doesn't support it */
#define SD_CMD_GO_IRQ_STATE                        ((cyg_uint8)40) /*!< SD Card doesn't support it */
#define SD_CMD_LOCK_UNLOCK                         ((cyg_uint8)42)
#define SD_CMD_APP_CMD                             ((cyg_uint8)55)
#define SD_CMD_GEN_CMD                             ((cyg_uint8)56)
#define SD_CMD_NO_CMD                              ((cyg_uint8)64)

/**
  * @brief Following commands are SD Card Specific commands.
  *        SDIO_APP_CMD should be sent before sending these commands.
  */
#define SD_CMD_APP_SD_SET_BUSWIDTH                 ((cyg_uint8)6)  /*!< For SD Card only */
#define SD_CMD_SD_APP_STAUS                        ((cyg_uint8)13) /*!< For SD Card only */
#define SD_CMD_SD_APP_SEND_NUM_WRITE_BLOCKS        ((cyg_uint8)22) /*!< For SD Card only */
#define SD_CMD_SD_APP_OP_COND                      ((cyg_uint8)41) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CLR_CARD_DETECT          ((cyg_uint8)42) /*!< For SD Card only */
#define SD_CMD_SD_APP_SEND_SCR                     ((cyg_uint8)51) /*!< For SD Card only */
#define SD_CMD_SDIO_RW_DIRECT                      ((cyg_uint8)52) /*!< For SD I/O Card only */
#define SD_CMD_SDIO_RW_EXTENDED                    ((cyg_uint8)53) /*!< For SD I/O Card only */

/**
  * @brief Following commands are SD Card Specific security commands.
  *        SDIO_APP_CMD should be sent before sending these commands.
  */
#define SD_CMD_SD_APP_GET_MKB                      ((cyg_uint8)43) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_MID                      ((cyg_uint8)44) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CER_RN1                  ((cyg_uint8)45) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_CER_RN2                  ((cyg_uint8)46) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CER_RES2                 ((cyg_uint8)47) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_CER_RES1                 ((cyg_uint8)48) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_READ_MULTIPLE_BLOCK   ((cyg_uint8)18) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_WRITE_MULTIPLE_BLOCK  ((cyg_uint8)25) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_ERASE                 ((cyg_uint8)38) /*!< For SD Card only */
#define SD_CMD_SD_APP_CHANGE_SECURE_AREA           ((cyg_uint8)49) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_WRITE_MKB             ((cyg_uint8)48) /*!< For SD Card only */


// ----------------------------------------------------------------------------
/**
  * @brief  Card Specific Data: CSD Register
  */
typedef struct {
  cyg_uint8   CSDStruct;            /*!< CSD structure */
  cyg_uint8   SysSpecVersion;       /*!< System specification version */
  cyg_uint8   Reserved1;            /*!< Reserved */
  cyg_uint8   TAAC;                 /*!< Data read access-time 1 */
  cyg_uint8   NSAC;                 /*!< Data read access-time 2 in CLK cycles */
  cyg_uint8   MaxBusClkFrec;        /*!< Max. bus clock frequency */
  cyg_uint16  CardComdClasses;      /*!< Card command classes */
  cyg_uint8   RdBlockLen;           /*!< Max. read data block length */
  cyg_uint8   PartBlockRead;        /*!< Partial blocks for read allowed */
  cyg_uint8   WrBlockMisalign;      /*!< Write block misalignment */
  cyg_uint8   RdBlockMisalign;      /*!< Read block misalignment */
  cyg_uint8   DSRImpl;              /*!< DSR implemented */
  cyg_uint8   Reserved2;            /*!< Reserved */
  cyg_uint32  DeviceSize;           /*!< Device Size */
  cyg_uint8   MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min */
  cyg_uint8   MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max */
  cyg_uint8   MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min */
  cyg_uint8   MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max */
  cyg_uint8   DeviceSizeMul;        /*!< Device size multiplier */
  cyg_uint8   EraseGrSize;          /*!< Erase group size */
  cyg_uint8   EraseGrMul;           /*!< Erase group size multiplier */
  cyg_uint8   WrProtectGrSize;      /*!< Write protect group size */
  cyg_uint8   WrProtectGrEnable;    /*!< Write protect group enable */
  cyg_uint8   ManDeflECC;           /*!< Manufacturer default ECC */
  cyg_uint8   WrSpeedFact;          /*!< Write speed factor */
  cyg_uint8   MaxWrBlockLen;        /*!< Max. write data block length */
  cyg_uint8   WriteBlockPaPartial;  /*!< Partial blocks for write allowed */
  cyg_uint8   Reserved3;            /*!< Reserded */
  cyg_uint8   ContentProtectAppli;  /*!< Content protection application */
  cyg_uint8   FileFormatGrouop;     /*!< File format group */
  cyg_uint8   CopyFlag;             /*!< Copy flag (OTP) */
  cyg_uint8   PermWrProtect;        /*!< Permanent write protection */
  cyg_uint8   TempWrProtect;        /*!< Temporary write protection */
  cyg_uint8   FileFormat;           /*!< File Format */
  cyg_uint8   ECC;                  /*!< ECC code */
  cyg_uint8   CSD_CRC;              /*!< CSD CRC */
  cyg_uint8   Reserved4;            /*!< always 1*/
} mmc_stm32_csd_t;

/**
  * @brief  Card Identification Data: CID Register
  */
typedef struct {
  cyg_uint8   ManufacturerID;       /*!< ManufacturerID */
  cyg_uint16  OEM_AppliID;          /*!< OEM/Application ID */
  cyg_uint32  ProdName1;            /*!< Product Name part1 */
  cyg_uint8   ProdName2;            /*!< Product Name part2*/
  cyg_uint8   ProdRev;              /*!< Product Revision */
  cyg_uint32  ProdSN;               /*!< Product Serial Number */
  cyg_uint8   Reserved1;            /*!< Reserved1 */
  cyg_uint16  ManufactDate;         /*!< Manufacturing Date */
  cyg_uint8   CID_CRC;              /*!< CID CRC */
  cyg_uint8   Reserved2;            /*!< always 1 */
} mmc_stm32_cid_t;


// ----------------------------------------------------------------------------

// The CID register is generally treated as an opaque data structure
// used only for unique identification of cards.
typedef struct mmc_cid_register {
    cyg_uint8   cid_data[16];
} mmc_cid_register;

#define MMC_CID_REGISTER_MID(_data_)                    ((_data_)->cid_data[0])
#define MMC_CID_REGISTER_OID(_data_)                    (((_data_)->cid_data[1] << 8) | \
                                                         ((_data_)->cid_data[2]))
#define MMC_CID_REGISTER_PNM(_data_)                    ((const char*)&((_data_)->cid_data[3]))
#define MMC_CID_REGISTER_PRV(_data_)                    ((_data_)->cid_data[9])
#define MMC_CID_REGISTER_PSN(_data_)                    (((_data_)->cid_data[10] << 24) | \
                                                         ((_data_)->cid_data[11] << 16) | \
                                                         ((_data_)->cid_data[12] <<  8) | \
                                                         ((_data_)->cid_data[13]))
#define MMC_CID_REGISTER_MDT(_data_)                    ((_data_)->cid_data[14])
#define MMC_CID_REGISTER_CRC(_data_)                    ((_data_)->cid_data[15] >> 1)



// The CSD register is just lots of small bitfields. For now keep it
// as an array of 16 bytes and provide macros to read the fields.
typedef struct mmc_csd_register {
    cyg_uint8   csd_data[16];
} mmc_csd_register;

#define MMC_CSD_REGISTER_CSD_STRUCTURE(_data_)          (((_data_)->csd_data[0] & 0x00C0) >> 6)
#define MMC_CSD_REGISTER_SPEC_VERS(_data_)              (((_data_)->csd_data[0] & 0x003C) >> 2)
#define MMC_CSD_REGISTER_TAAC_MANTISSA(_data_)          (((_data_)->csd_data[1] & 0x0078) >> 3)
#define MMC_CSD_REGISTER_TAAC_EXPONENT(_data_)          ((_data_)->csd_data[1] & 0x0007)
#define MMC_CSD_REGISTER_NSAC(_data_)                   ((_data_)->csd_data[2])
#define MMC_CSD_REGISTER_TRAN_SPEED_MANTISSA(_data_)    (((_data_)->csd_data[3] & 0x0078) >> 3)
#define MMC_CSD_REGISTER_TRAN_SPEED_EXPONENT(_data_)    ((_data_)->csd_data[3] & 0x0007)
#define MMC_CSD_REGISTER_CCC(_data_)                    (((_data_)->csd_data[4] << 4) | (((_data_)->csd_data[5] & 0x00F0) >> 4))
#define MMC_CSD_REGISTER_READ_BL_LEN(_data_)            ((_data_)->csd_data[5] & 0x000F)
#define MMC_CSD_REGISTER_READ_BL_PARTIAL(_data_)        (((_data_)->csd_data[6] & 0x0080) >> 7)
#define MMC_CSD_REGISTER_WRITE_BLK_MISALIGN(_data_)     (((_data_)->csd_data[6] & 0x0040) >> 6)
#define MMC_CSD_REGISTER_READ_BLK_MISALIGN(_data_)      (((_data_)->csd_data[6] & 0x0020) >> 5)
#define MMC_CSD_REGISTER_DSR_IMP(_data_)                (((_data_)->csd_data[6] & 0x0010) >> 4)
#define MMC_CSD_REGISTER_C_SIZE(_data_)                 ((((_data_)->csd_data[6] & 0x0003) << 10) |     \
                                                          ((_data_)->csd_data[7] << 2)            |     \
                                                          (((_data_)->csd_data[8] & 0x00C0) >> 6))
#define MMC_CSD_REGISTER_VDD_R_CURR_MIN(_data_)         (((_data_)->csd_data[8] & 0x0038)  >> 3)
#define MMC_CSD_REGISTER_VDD_R_CURR_MAX(_data_)         ((_data_)->csd_data[8] & 0x0007)
#define MMC_CSD_REGISTER_VDD_W_CURR_MIN(_data_)         (((_data_)->csd_data[9] & 0x00E0) >> 5)
#define MMC_CSD_REGISTER_VDD_W_CURR_MAX(_data_)         (((_data_)->csd_data[9] & 0x001C) >> 2)
#define MMC_CSD_REGISTER_C_SIZE_MULT(_data_)            ((((_data_)->csd_data[9] & 0x0003) << 1) |      \
                                                         (((_data_)->csd_data[10] & 0x0080) >> 7))
#define MMC_CSD_REGISTER_SECTOR_SIZE(_data_)            (((_data_)->csd_data[10] & 0x007C) >> 2)
#define MMC_CSD_REGISTER_ERASE_GRP_SIZE(_data_)         ((((_data_)->csd_data[10] & 0x0003) << 3) |     \
                                                         (((_data_)->csd_data[11] & 0x00E0) >> 5))
#define MMC_CSD_REGISTER_WR_GRP_SIZE(_data_)            ((_data_)->csd_data[11] & 0x001F)
#define MMC_CSD_REGISTER_WR_GRP_ENABLE(_data_)          (((_data_)->csd_data[12] & 0x0080) >> 7)
#define MMC_CSD_REGISTER_DEFAULT_ECC(_data_)            (((_data_)->csd_data[12] & 0x0060) >> 5)
#define MMC_CSD_REGISTER_R2W_FACTOR(_data_)             (((_data_)->csd_data[12] & 0x001C) >> 2)
#define MMC_CSD_REGISTER_WRITE_BL_LEN(_data_)           ((((_data_)->csd_data[12] & 0x0003) << 2) |     \
                                                         (((_data_)->csd_data[13] & 0x00C0) >> 6))
#define MMC_CSD_REGISTER_WR_BL_PARTIAL(_data_)          (((_data_)->csd_data[13] & 0x0020) >> 5)
#define MMC_CSD_REGISTER_FILE_FORMAT_GROUP(_data_)      (((_data_)->csd_data[14] & 0x0080) >> 7)
#define MMC_CSD_REGISTER_COPY(_data_)                   (((_data_)->csd_data[14] & 0x0040) >> 6)
#define MMC_CSD_REGISTER_PERM_WRITE_PROTECT(_data_)     (((_data_)->csd_data[14] & 0x0020) >> 5)
#define MMC_CSD_REGISTER_TMP_WRITE_PROTECT(_data_)      (((_data_)->csd_data[14] & 0x0010) >> 4)
#define MMC_CSD_REGISTER_FILE_FORMAT(_data_)            (((_data_)->csd_data[14] & 0x000C) >> 2)
#define MMC_CSD_REGISTER_ECC(_data_)                    ((_data_)->csd_data[14] & 0x0003)
#define MMC_CSD_REGISTER_CRC(_data_)                    (((_data_)->csd_data[15] & 0xFE) >> 1)




// ----------------------------------------------------------------------------
//

typedef struct {
    cyg_haladdress    sdio_reg_base;   // Base address of SDIO register block.

    cyg_priority_t    dma_tx_intr_pri; // Interrupt priority for DMA transmit.
    cyg_priority_t    dma_rx_intr_pri; // Interrupt priority for DMA receive.
    cyg_uint32        bbuf_size;       // Size of bounce buffers.
    cyg_uint8*        bbuf;            // Pointer to bounce buffer.
} cyg_stm32_sdio_setup_t;

// Details of a specific MMC card
typedef struct cyg_mmc_stm32_disk_info_t {
    // ---- SDIO configuration constants ----
    const cyg_stm32_sdio_setup_t * setup;

    /**
     * @brief SD Card information
     */
    mmc_stm32_csd_t     csd;
    mmc_stm32_cid_t     cid;
    sd_card_type_t      card_type;
    cyg_uint32          capacity;  /*!< Card Capacity */
    cyg_uint32          blocksize; /*!< Card Block Size */
    cyg_uint16          rca;


    cyg_uint32          mmc_block_count;
#ifdef MMC_STM32_BACKGROUND_WRITES
    cyg_bool            mmc_writing;
#endif
    cyg_bool            mmc_read_only;
    cyg_bool            mmc_connected;
    cyg_uint32          mmc_heads_per_cylinder;
    cyg_uint32          mmc_sectors_per_head;
    cyg_uint32          mmc_read_block_length;
    cyg_uint32          mmc_write_block_length;


    // ---- Driver state (private) ----
    hal_stm32_dma_stream dma_tx_stream;// TX DMA stream for SDIO.
    hal_stm32_dma_stream dma_rx_stream;// RX DMA stream for SDIO.
    cyg_drv_mutex_t      mutex;        // Transfer mutex.
    cyg_drv_cond_t       condvar;      // Transfer condition variable.
    cyg_bool             dma_done;     // Flags used to signal completion.

    // ---- Internal private ----
    cyg_mutex_t          wr_mutex;     // Transfer mutex for write and read.

    // Runtime state for DMA transfer
    cyg_uint8*           buf;
    cyg_uint32           count;
    cyg_bool             is_tx;
    cyg_bool             polled;
} cyg_mmc_stm32_disk_info_t;


#endif // #endif CYGONCE_DEVS_DISK_CORTEXM_STM32_MMC_H

// ----------------------------------------------------------------------------
// EOF mmc_stm32.h
