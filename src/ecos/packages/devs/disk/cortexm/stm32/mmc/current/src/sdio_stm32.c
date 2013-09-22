//==========================================================================
//
//      mmc_stm32.c
//
//      STM32 SDIO Peripheral driver
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2004, 2006 Free Software Foundation, Inc.
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
// Author:       bartv
// Date:         2013-06-29
//
//####DESCRIPTIONEND####
//==========================================================================

#include <pkgconf/system.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_if.h>             // delays
#include <cyg/hal/hal_intr.h>
#include <string.h>
#include <errno.h>


// Communication parameters. First some debug support
#define DEBUG   0
#if DEBUG > 0
# define DEBUG1(format, ...)    diag_printf(format, ## __VA_ARGS__)
#else
# define DEBUG1(format, ...)
#endif
#if DEBUG > 1
# define DEBUG2(format, ...)    diag_printf(format, ## __VA_ARGS__)
#else
# define DEBUG2(format, ...)
#endif

#if 0
// ----------------------------------------------------------------------------

static cyg_uint8 stm32_sdio_get_status_flag(cyg_uint32 flag_bit)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 reg_data;

    HAL_READ_UINT32 (reg_addr, reg_data);
    return  (reg_data & flag_bit);
}

static void stm32_sdio_clear_status_flag(cyg_uint32 flag_bit)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_ICR;
    cyg_uint32 reg_data = flag_bit;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

/**
  * @brief  Returns command index of last command for which response received.
  * @param  None
  * @retval Returns the command index of the last command response received.
  */
static cyg_uint8 stm32_sdio_get_command_response(void)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_RESPCMD;
    cyg_uint32 reg_data;

    HAL_READ_UINT32 (reg_addr, reg_data);
    return  ((cyg_uint8)reg_data);
}

/**
  * @brief  Returns response received from the card for the last command.
  * @param  index: Specifies the SDIO response register.
  *   This parameter can be one of the following values:
  *     @arg 1: Response Register 1
  *     @arg 2: Response Register 2
  *     @arg 3: Response Register 3
  *     @arg 4: Response Register 4
  * @retval The Corresponding response register value.
  */
cyg_uint32 stm32_sdio_get_response(cyg_uint8 index)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_RESP((index-1));
    cyg_uint32 reg_data;

    HAL_READ_UINT32 (reg_addr, reg_data);
    return  reg_data;
}

// ----------------------------------------------------------------------------

void stm32_sdio_init(void)
{
    /*!< Configure the SDIO peripheral */
    /*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_INIT_CLK_DIV) */
    /*!< SDIO_CK for initialization should not exceed 400 KHz */

    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    cyg_uint32 reg_data;

    /*------------------------- SDIO CLKCR Configuration ---------------------*/
    /* Get the SDIO CLKCR value */
    HAL_READ_UINT32 (reg_addr, reg_data);

    /* Clear CLKDIV, PWRSAV, BYPASS, WIDBUS, NEGEDGE, HWFC_EN bits */
    reg_data &= CYGHWR_HAL_STM32_SDIO_CLKCR_CLEAR_MASK;

    /* Set CLKDIV bits, other bits keep the default configuration:
     * PWRSAV, BYPASS, HWFC_EN : Disable;
     * NEGEDGE : Rising;
     * WIDBUS  : 1bit;
     */
    reg_data |= CYGHWR_HAL_STM32_SDIO_CLKCR_CLKDIV(STM32_SDIO_INIT_CLK_DIV);

    /* Write to SDIO CLKCR */
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

// Deinitializes the SDIO peripheral registers to their default reset values.
void stm32_sdio_deinit(void)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data = 0x00000000;

    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_POWER;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_ARG;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CMD;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DTIMER;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DLEN;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DCTRL;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_MASK;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    reg_data = 0x00C007FF;
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_ICR;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

void stm32_sdio_set_clk_div(cyg_uint8 clk_div)
{
#define STM32_SDIO_CLKCR_CLK_DIV_CLEAR_MASK 0xFFFFFFF1

    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    cyg_uint32 reg_data;

    /* Get the SDIO CLKCR value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    reg_data &= STM32_SDIO_CLKCR_CLK_DIV_CLEAR_MASK;
    reg_data |= CYGHWR_HAL_STM32_SDIO_CLKCR_CLKDIV(clk_div);
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

void stm32_sdio_set_bus_wide(cyg_uint32 bus_wide)
{
#define STM32_SDIO_CLKCR_BUS_WIDE_CLEAR_MASK ((cyg_uint32)(3 << 11))

    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    cyg_uint32 reg_data;

    /* Get the SDIO CLKCR value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    reg_data &= STM32_SDIO_CLKCR_BUS_WIDE_CLEAR_MASK;
    if (bus_wide == SDIO_BusWide_1b) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_CLKCR_WIDBUS_1BIT;
    } else if (bus_wide == SDIO_BusWide_4b) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_CLKCR_WIDBUS_4BIT;
    } else if (bus_wide == SDIO_BusWide_8b) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_CLKCR_WIDBUS_8BIT;
    } else {
        // TODO:
    }
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

void stm32_sdio_set_power_state(cyg_bool power_on)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_POWER;
    reg_data = power_on ? CYGHWR_HAL_STM32_SDIO_POWER_PWRON:
                          CYGHWR_HAL_STM32_SDIO_POWER_PWROFF;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

void stm32_sdio_get_power_state(cyg_bool * power_on)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_POWER;
    cyg_uint32 reg_data;
    HAL_READ_UINT32 (reg_addr, reg_data);
    *power_on = (reg_data & CYGHWR_HAL_STM32_SDIO_POWER_PWRON) ? true : false;
}

void stm32_sdio_clk_enable(cyg_bool enable)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    cyg_uint32 reg_data;

    /* Get the SDIO CLKCR value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    if (enable) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_CLKCR_CLKEN;
    } else {
        reg_data &= (~CYGHWR_HAL_STM32_SDIO_CLKCR_CLKEN);
    }
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

void stm32_sdio_send_command(stm32_sdio_cmd_t * cmd)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    /* Check the parameters */ /*
    assert_param(IS_SDIO_CMD_INDEX(SDIO_CmdInitStruct->SDIO_CmdIndex));
    assert_param(IS_SDIO_RESPONSE(SDIO_CmdInitStruct->SDIO_Response));
    assert_param(IS_SDIO_WAIT(SDIO_CmdInitStruct->SDIO_Wait));
    assert_param(IS_SDIO_CPSM(SDIO_CmdInitStruct->SDIO_CPSM)); */

    /*-------------------------- SDIO ARG Configuration ----------------------*/
    /* Set the SDIO Argument value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_ARG;
    reg_data = cmd->argument;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    /*-------------------------- SDIO CMD Configuration ----------------------*/
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CMD;
    /* Get the SDIO CMD value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    /* Clear CMDINDEX, WAITRESP, WAITINT, WAITPEND, CPSMEN bits */
    reg_data &= CYGHWR_HAL_STM32_SDIO_CMD_CLEAR_MASK;
    /* Set CMDINDEX bits according to SDIO_CmdIndex value */
    /* Set WAITRESP bits according to SDIO_Response value */
    /* Set WAITINT and WAITPEND bits according to SDIO_Wait value */
    /* Set CPSMEN bits according to SDIO_CPSM value */
    reg_data |= (cmd->cmd_index | cmd->response | cmd->wait | cmd->cpsm);

    /* Write to SDIO CMD */
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

void stm32_sdio_data_config(cyg_uint32 data_timeout, cyg_uint32 data_len,
        cyg_uint8 block_size, cyg_bool to_card, cyg_bool block_mode, cyg_bool dpsm_en)
{
/* SDIO DCTRL Clear Mask */
#define DCTRL_CLEAR_MASK         ((cyg_uint32)0xFFFFFF08)

    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    /* Check the parameters */ /*
    assert_param(IS_SDIO_DATA_LENGTH(SDIO_DataInitStruct->SDIO_DataLength));
    assert_param(IS_SDIO_BLOCK_SIZE(SDIO_DataInitStruct->SDIO_DataBlockSize));
    assert_param(IS_SDIO_TRANSFER_DIR(SDIO_DataInitStruct->SDIO_TransferDir));
    assert_param(IS_SDIO_TRANSFER_MODE(SDIO_DataInitStruct->SDIO_TransferMode));
    assert_param(IS_SDIO_DPSM(SDIO_DataInitStruct->SDIO_DPSM)); */

    /*-------------------------- SDIO DTIMER Configuration -------------------*/
    /* Set the SDIO Data TimeOut value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DTIMER;
    reg_data = data_timeout;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    /*-------------------------- SDIO DLEN Configuration ---------------------*/
    /* Set the SDIO DataLength value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DLEN;
    reg_data = data_len & 0x00FFFFFF;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    /*-------------------------- SDIO DCTRL Configuration --------------------*/
    /* Get the SDIO DCTRL value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DCTRL;
    HAL_READ_UINT32 (reg_addr, reg_data);
    /* Clear DEN, DTMODE, DTDIR and DBCKSIZE bits */
    reg_data &= DCTRL_CLEAR_MASK;
    /* Set DEN, DTMODE, DTDIR and DBCKSIZE bits */
    int i = 0;
    for (i = 0; i < 0xF; i++) {
        if ((1 << i) == block_size) {
            break;
        }
    }
    if (i == 0xF) {
        return;
    } else {
        reg_data |= CYGHWR_HAL_STM32_SDIO_DCTRL_DBLOCKSIZE(i);
    }
    if (!to_card) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_DCTRL_DTDIR;
    }
    if (!block_mode) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_DCTRL_DMODE_STREAM;
    }
    if (dpsm_en) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_DCTRL_DTEN;
    }

    /* Write to SDIO DCTRL */
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

/**
  * @brief  Read one data word from Rx FIFO.
  * @param  None
  * @retval Data received
  */
cyg_uint32 stm32_sdio_read_data(void)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_FIFO;
    cyg_uint32 reg_data;
    HAL_READ_UINT32 (reg_addr, reg_data);
    return reg_data;
}

// ----------------------------------------------------------------------------

// Checks for error conditions for CMD0.
stm32_sd_error_t stm32_sdio_chk_cmd0_error(void)
{
    cyg_uint32 timeout = 0x00010000; /*!< 10000 */

    while ((timeout > 0) &&
            !stm32_sdio_get_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CMDSENT)) {
        timeout--;
    }

    if (timeout == 0) {
        return  SD_CMD_RSP_TIMEOUT;
    }

    /*!< Clear all the static flags */
    stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CMDSENT);
    return  SD_OK;
}

// Checks for error conditions for R7 response.
stm32_sd_error_t stm32_sdio_chk_cmd_resp7_error(void)
{
    cyg_uint32 timeout = 0x00010000; /*!< 10000 */
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 reg_data;

    HAL_READ_UINT32 (reg_addr, reg_data);
    while ((timeout > 0) && !(reg_data &
                (CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL |
                 CYGHWR_HAL_STM32_SDIO_STA_CMDREND  |
                 CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT))) {
        timeout--;
        HAL_READ_UINT32 (reg_addr, reg_data);
    }

    if (timeout == 0 || (reg_data & CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT)) {
        /*!< Card is not V2.0 complient or card does not support the set voltage range */
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT);
        return  SD_CMD_RSP_TIMEOUT;
    }

    if ((reg_data & CYGHWR_HAL_STM32_SDIO_STA_CMDREND)) {
        /*!< Card is SD V2.0 compliant */
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CMDREND);
    }

    return  SD_OK;
}

// @brief           Checks for error conditions for R1 response.
// @param  cmd:     The sent command index.
// @retval stm32_sd_error_t: SD Card Error code.
stm32_sd_error_t stm32_sdio_chk_cmd_resp1_error(cyg_uint8 cmd)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 status;
    cyg_uint32 response_r1;

    do {
        HAL_READ_UINT32 (reg_addr, status);
    } while (!(status & (CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL |
                         CYGHWR_HAL_STM32_SDIO_STA_CMDREND  |
                         CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT)));

    if (status & CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT);
        return  SD_CMD_RSP_TIMEOUT;
    } else if (status & CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL);
        return  SD_CMD_CRC_FAIL;
    }

    /*!< Check response received is of desired command */
    if (stm32_sdio_get_command_response() != cmd) {
        return  SD_ILLEGAL_CMD;
    }

    /*!< Clear all the static flags */
    stm32_sdio_clear_status_flag(STM32_SDIO_STA_REG_STATIC_FLAGS);

    /*!< We have received response, retrieve it for analysis  */
    response_r1 = stm32_sdio_get_response(1);

    if ((response_r1 & SD_OCR_ERRORBITS) == SD_ALLZERO) {
        return  SD_OK;
    }

    if (response_r1 & SD_OCR_ADDR_OUT_OF_RANGE) {
        return  SD_ADDR_OUT_OF_RANGE;
    }

    if (response_r1 & SD_OCR_ADDR_MISALIGNED) {
        return  SD_ADDR_MISALIGNED;
    }

    if (response_r1 & SD_OCR_BLOCK_LEN_ERR) {
        return  SD_BLOCK_LEN_ERR;
    }

    if (response_r1 & SD_OCR_ERASE_SEQ_ERR) {
        return  SD_ERASE_SEQ_ERR;
    }

    if (response_r1 & SD_OCR_BAD_ERASE_PARAM) {
        return  SD_BAD_ERASE_PARAM;
    }

    if (response_r1 & SD_OCR_WRITE_PROT_VIOLATION) {
        return  SD_WRITE_PROT_VIOLATION;
    }

    if (response_r1 & SD_OCR_LOCK_UNLOCK_FAILED) {
        return  SD_LOCK_UNLOCK_FAILED;
    }

    if (response_r1 & SD_OCR_COM_CRC_FAILED) {
        return  SD_COM_CRC_FAILED;
    }

    if (response_r1 & SD_OCR_ILLEGAL_CMD) {
        return  SD_ILLEGAL_CMD;
    }

    if (response_r1 & SD_OCR_CARD_ECC_FAILED) {
        return  SD_CARD_ECC_FAILED;
    }

    if (response_r1 & SD_OCR_CC_ERROR) {
        return  SD_CC_ERROR;
    }

    if (response_r1 & SD_OCR_GENERAL_UNKNOWN_ERROR) {
        return  SD_GENERAL_UNKNOWN_ERROR;
    }

    if (response_r1 & SD_OCR_STREAM_READ_UNDERRUN) {
        return  SD_STREAM_READ_UNDERRUN;
    }

    if (response_r1 & SD_OCR_STREAM_WRITE_OVERRUN) {
        return  SD_STREAM_WRITE_OVERRUN;
    }

    if (response_r1 & SD_OCR_CID_CSD_OVERWRIETE) {
        return  SD_CID_CSD_OVERWRITE;
    }

    if (response_r1 & SD_OCR_WP_ERASE_SKIP) {
        return  SD_WP_ERASE_SKIP;
    }

    if (response_r1 & SD_OCR_CARD_ECC_DISABLED) {
        return  SD_CARD_ECC_DISABLED;
    }

    if (response_r1 & SD_OCR_ERASE_RESET) {
        return  SD_ERASE_RESET;
    }

    if (response_r1 & SD_OCR_AKE_SEQ_ERROR) {
        return  SD_AKE_SEQ_ERROR;
    }

    return  SD_OK;
}

/**
  * @brief  Checks for error conditions for R2 (CID or CSD) response.
  * @param  None
  * @retval stm32_sd_error_t: SD Card Error code.
  */
stm32_sd_error_t stm32_sdio_chk_cmd_resp2_error(void)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 status;

    do {
        HAL_READ_UINT32 (reg_addr, status);
    } while (!(status & (CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL |
                         CYGHWR_HAL_STM32_SDIO_STA_CMDREND  |
                         CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT)));

    if (status & CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT);
        return  SD_CMD_RSP_TIMEOUT;
    } else if (status & CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL);
        return  SD_CMD_CRC_FAIL;
    }

    /*!< Clear all the static flags */
    stm32_sdio_clear_status_flag(STM32_SDIO_STA_REG_STATIC_FLAGS);
    return SD_OK;
}

/**
  * @brief  Checks for error conditions for R3 (OCR) response.
  * @param  None
  * @retval stm32_sd_error_t: SD Card Error code.
  */
stm32_sd_error_t stm32_sdio_chk_cmd_resp3_error(void)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 status;

    do {
        HAL_READ_UINT32 (reg_addr, status);
    } while (!(status & (CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL |
                         CYGHWR_HAL_STM32_SDIO_STA_CMDREND  |
                         CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT)));

    if (status & CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT);
        return  SD_CMD_RSP_TIMEOUT;
    }
    /*!< Clear all the static flags */
    stm32_sdio_clear_status_flag(STM32_SDIO_STA_REG_STATIC_FLAGS);
    return SD_OK;
}

/**
  * @brief  Checks for error conditions for R6 (RCA) response.
  * @param  cmd: The sent command index.
  * @param  prca: pointer to the variable that will contain the SD card relative
  *         address RCA.
  * @retval stm32_sd_error_t: SD Card Error code.
  */
stm32_sd_error_t
stm32_sdio_chk_cmd_resp6_error(cyg_uint8 cmd, cyg_uint16 * prca)
{
/**
  * @brief  Masks for R6 Response
  */
#define SD_R6_GENERAL_UNKNOWN_ERROR     ((cyg_uint32)0x00002000)
#define SD_R6_ILLEGAL_CMD               ((cyg_uint32)0x00004000)
#define SD_R6_COM_CRC_FAILED            ((cyg_uint32)0x00008000)

    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 status;
    cyg_uint32 response_r1;

    do {
        HAL_READ_UINT32 (reg_addr, status);
    } while (!(status & (CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL |
                       CYGHWR_HAL_STM32_SDIO_STA_CMDREND  |
                       CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT)));

    if (status & CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CTIMEOUT);
        return  SD_CMD_RSP_TIMEOUT;
    } else if (status & CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CCRCFAIL);
        return  SD_CMD_CRC_FAIL;
    }

    /*!< Check response received is of desired command */
    if (stm32_sdio_get_command_response() != cmd) {
        return  SD_ILLEGAL_CMD;
    }

    /*!< Clear all the static flags */
    stm32_sdio_clear_status_flag(STM32_SDIO_STA_REG_STATIC_FLAGS);

    /*!< We have received response, retrieve it.  */
    response_r1 = stm32_sdio_get_response(1);

    if (SD_ALLZERO == (response_r1 &
        (SD_R6_GENERAL_UNKNOWN_ERROR | SD_R6_ILLEGAL_CMD | SD_R6_COM_CRC_FAILED))) {
        *prca = (cyg_uint16) (response_r1 >> 16);
        return  SD_OK;
    }

    if (response_r1 & SD_R6_GENERAL_UNKNOWN_ERROR) {
        return  SD_GENERAL_UNKNOWN_ERROR;
    }

    if (response_r1 & SD_R6_ILLEGAL_CMD) {
        return  SD_ILLEGAL_CMD;
    }

    if (response_r1 & SD_R6_COM_CRC_FAILED) {
        return  SD_COM_CRC_FAILED;
    }

    return  SD_OK;
}

/**
  * @brief  Find the SD card SCR register value.
  * @param  rca: selected card address.
  * @param  pscr: pointer to the buffer that will contain the SCR value.
  * @retval stm32_sd_error_t: SD Card Error code.
  */
stm32_sd_error_t stm32_sdio_find_scr(cyg_uint16 rca, cyg_uint32 * pscr)
{
    cyg_haladdress reg_addr;
    stm32_sd_error_t errorstatus = SD_OK;
    stm32_sdio_cmd_t sd_cmd;
    stm32_sdio_data_config_t data_config;
    cyg_uint32 tempscr[2] = {0, 0};
    cyg_uint32 index = 0;
    cyg_uint32 status;

    /*!< Set Block Size To 8 Bytes */
    /*!< Send CMD55 APP_CMD with argument as card's RCA */
    sd_cmd.argument = (cyg_uint32)8;/* Argument */
    sd_cmd.cmd_index= SD_CMD_SET_BLOCKLEN;/* Command */
    sd_cmd.response = CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND;
    sd_cmd.wait     = CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE;
    sd_cmd.cpsm     = CYGHWR_HAL_STM32_SDIO_CMD_CPSMEN);
    stm32_sdio_send_command(&sd_cmd);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_SET_BLOCKLEN);
    if (errorstatus != SD_OK) {
        return errorstatus;
    }

    /*!< Send CMD55 APP_CMD with argument as card's RCA */
    sd_cmd.argument = (cyg_uint32) rca << 16;/* Argument */
    sd_cmd.cmd_index= SD_CMD_APP_CMD;/* Command */
    sd_cmd.response = CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND;
    sd_cmd.wait     = CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE;
    sd_cmd.cpsm     = CYGHWR_HAL_STM32_SDIO_CMD_CPSMEN);
    stm32_sdio_send_command(&sd_cmd);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_CMD);
    if (errorstatus != SD_OK) {
        return errorstatus;
    }

    stm32_sdio_data_config(SD_DATATIMEOUT, 8, 8, 0, 1, 1);

    /*!< Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
    sd_cmd.argument = 0x0;/* Argument */
    sd_cmd.cmd_index= SD_CMD_SD_APP_SEND_SCR;/* Command */
    sd_cmd.response = CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND;
    sd_cmd.wait     = CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE;
    sd_cmd.cpsm     = CYGHWR_HAL_STM32_SDIO_CMD_CPSMEN);
    stm32_sdio_send_command(&sd_cmd);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_SD_APP_SEND_SCR);
    if (errorstatus != SD_OK) {
        return errorstatus;
    }

    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    HAL_READ_UINT32 (reg_addr, status);
    while (!(status & (CYGHWR_HAL_STM32_SDIO_STA_RXOVERR |
        CYGHWR_HAL_STM32_SDIO_STA_DCRCFAIL | CYGHWR_HAL_STM32_SDIO_STA_DTIMEOUT) |
        CYGHWR_HAL_STM32_SDIO_STA_DBCKEND | CYGHWR_HAL_STM32_SDIO_STA_STBITERR)) {
        if (status & CYGHWR_HAL_STM32_SDIO_STA_RXDVAL) {
            // Data available in receive FIFO
            *(tempscr + index) = stm32_sdio_read_data();
            index++;
        }
        HAL_READ_UINT32 (reg_addr, status);
    }

    if (status & CYGHWR_HAL_STM32_SDIO_STA_DTIMEOUT) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_DTIMEOUT);
        return  SD_CMD_RSP_TIMEOUT;
    } else if (status & CYGHWR_HAL_STM32_SDIO_STA_DCRCFAIL) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_DCRCFAIL);
        return  SD_CMD_CRC_FAIL;
    } else if (status & CYGHWR_HAL_STM32_SDIO_STA_RXOVERR) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_RXOVERR);
        return  SD_RX_OVERRUN;
    } else if (status & CYGHWR_HAL_STM32_SDIO_STA_STBITERR) {
        stm32_sdio_clear_status_flag(CYGHWR_HAL_STM32_SDIO_STA_STBITERR);
        return  SD_START_BIT_ERR;
    }

    /*!< Clear all the static flags */
    stm32_sdio_clear_status_flag(STM32_SDIO_STA_REG_STATIC_FLAGS);

    *(pscr + 1) = ((tempscr[0] & SD_0TO7BITS) << 24) |
                  ((tempscr[0] & SD_8TO15BITS) << 8) |
                  ((tempscr[0] & SD_16TO23BITS) >> 8) |
                  ((tempscr[0] & SD_24TO31BITS) >> 24);

    *(pscr) = ((tempscr[1] & SD_0TO7BITS) << 24) |
              ((tempscr[1] & SD_8TO15BITS) << 8) |
              ((tempscr[1] & SD_16TO23BITS) >> 8) |
              ((tempscr[1] & SD_24TO31BITS) >> 24);

    return  SD_OK;
}
#endif

// ----------------------------------------------------------------------------
// EOF sdio_stm32.c
