//==========================================================================
//
//      sdio_stm32.h
//
//      STM32 SDIO Peripheral driver
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
// Date:          2013-06-29
// Description:   STM32 SDIO Peripheral driver
//####DESCRIPTIONEND####
//==========================================================================

#ifndef CYGONCE_DEVS_DISK_CORTEXM_STM32_MMC_SDIO_H
#define CYGONCE_DEVS_DISK_CORTEXM_STM32_MMC_SDIO_H

#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/hal/drv_api.h>

#if 0
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

// SDIO CMD register clear mask
#define CYGHWR_HAL_STM32_SDIO_CMD_CLEAR_MASK        MASK_(15, 17)
/* CLKCR register clear mask */
#define CYGHWR_HAL_STM32_SDIO_CLKCR_CLEAR_MASK      ((cyg_uint32)0xFFFF8100)


/* SDIO Static flags for SDIO STA register */
#define STM32_SDIO_STA_REG_STATIC_FLAGS             ((cyg_uint32)0x000005FF)


/**
  * @brief  SDIO Intialization Frequency (400KHz max)
  */
#define STM32_SDIO_INIT_CLK_DIV                     ((cyg_uint8)0xB2)
/**
  * @brief  SDIO Data Transfer Frequency (25MHz max)
  */
#define STM32_SDIO_TRANSFER_CLK_DIV                 ((cyg_uint8)0x2)



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

#endif

#endif // #endif CYGONCE_DEVS_DISK_CORTEXM_STM32_MMC_SDIO_H

// ----------------------------------------------------------------------------
// EOF sdio_stm32.h
