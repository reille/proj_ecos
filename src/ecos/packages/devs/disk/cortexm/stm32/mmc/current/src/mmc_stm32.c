//==========================================================================
//
//      mmc_stm32.c
//
//      Provide a MMC driver for STM32 CortexM processors over SDIO
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
// Date:         2013-06-26
//
//####DESCRIPTIONEND####
//==========================================================================

#include <pkgconf/devs_disk_cortexm_stm32_mmc.h>
#include <pkgconf/io.h>

#include <pkgconf/system.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_if.h>             // delays
#include <cyg/hal/hal_intr.h>
#include <string.h>
#include <errno.h>
#include <cyg/io/io.h>
#include <cyg/io/devtab.h>
#include <cyg/io/disk.h>

#include <cyg/io/mmc_stm32.h>

#include <string.h>/* for memcpy, memset and so on */


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


// Should write operations be allowed to complete in the background,
// or must the operation complete in the foreground. The latter
// requires polling for potentially a long time, up to some 100's of
// milliseconds, but the former appears unreliable if there are other
// devices on the SPI bus. In theory the MMC card should detect that
// the chip select line is dropped and tristate the output line, but
// in practice this does not always happen.
#undef MMC_STM32_BACKGROUND_WRITES

// The size of each disk block
#define MMC_STM32_BLOCK_SIZE      512


// ----------------------------------------------------------------------------

static cyg_uint32 CSD_Tab[4], CID_Tab[4];


//-----------------------------------------------------------------------------
// DMA Callbacks. The DMA driver has disabled the DMA channel and
// masked the interrupt condition, here we need to wake up the client
// thread.

static void stm32_dma_callback( hal_stm32_dma_stream *stream,
                                      cyg_uint32 count, CYG_ADDRWORD data );


//-----------------------------------------------------------------------------
// Null data source and sink must be placed in the on-chip SRAM.  This is
// either done explicitly (bounce buffers instantiated) or implicitly (no
// bounce buffers implies that the data area is already on SRAM).

#if (defined (CYGHWR_DEVS_DISK_CORTEXM_STM32_MMC_DISK0) && \
             (CYGNUM_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_BBUF_SIZE > 0))
static cyg_uint32 dma_null __attribute__((section (".sram"))) = 0xFFFFFFFF;
#else
static cyg_uint32 dma_null = 0xFFFFFFFF;
#endif


// ----------------------------------------------------------------------------

#define SD_DETECT_IO( __port, __bit ) CYGHWR_HAL_STM32_PIN_IN(__port, __bit, PULLUP)

#define STM32_MMC_CARD_DETECT_PIN  CYGHWR_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_DETECT_GPIO

static cyg_uint8 disk0_bbuf [CYGNUM_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_BBUF_SIZE]
  __attribute__((aligned (4), section (".sram"))) = { 0 };


static const cyg_stm32_sdio_setup_t stm32_sdio_setup = {
    .sdio_reg_base      = CYGHWR_HAL_STM32_SDIO,

    .dma_tx_intr_pri    = CYGNUM_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_DMA_TXINTR_PRI,
    .dma_rx_intr_pri    = CYGNUM_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_DMA_RXINTR_PRI,

#if (CYGNUM_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_BBUF_SIZE > 0)
    .bbuf_size          = CYGNUM_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_BBUF_SIZE,
    .bbuf               = disk0_bbuf,
#else
    .bbuf_size          = 0,
#endif
};


// ----------------------------------------------------------------------------

/**
  * @brief  Converts the number of bytes in power of two and returns the power.
  * @param  NumberOfBytes: number of bytes.
  * @retval None
  */
static cyg_uint8
convert_from_bytes_to_power_of_two(cyg_uint16 NumberOfBytes)
{
    cyg_uint8 count = 0;

    while (NumberOfBytes != 1) {
        NumberOfBytes >>= 1;
        count++;
    }
    return(count);
}



// ----------------------------------------------------------------------------
// --------------------------- SDIO Driver ------------------------------------
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------

static
cyg_uint8 stm32_sdio_get_status_flag(cyg_uint32 flag_bit)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 reg_data;

    HAL_READ_UINT32 (reg_addr, reg_data);
    return  (reg_data & flag_bit);
}

static
void stm32_sdio_clear_status_flag(cyg_uint32 flag_bit)
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
static
cyg_uint8 stm32_sdio_get_command_response(void)
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
static
cyg_uint32 stm32_sdio_get_response(cyg_uint8 index)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_RESP((index-1));
    cyg_uint32 reg_data;

    HAL_READ_UINT32 (reg_addr, reg_data);
    return  reg_data;
}

// ----------------------------------------------------------------------------

static
void stm32_sdio_init(void)
{
/* CLKCR register clear mask */
#define CYGHWR_HAL_STM32_SDIO_CLKCR_CLEAR_MASK      ((cyg_uint32)0xFFFF8100)

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
     *  PWRSAV, BYPASS, HWFC_EN : Disable;
     *  NEGEDGE : Rising;
     *  WIDBUS  : 1bit;
     */
    reg_data |= STM32_SDIO_TRANSFER_CLK_DIV_400KHz;


    /* Write to SDIO CLKCR */
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

// Deinitializes the SDIO peripheral registers to their default reset values.
static
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

static
void stm32_sdio_set_clk_div(cyg_uint8 clk_div)
{
#define STM32_SDIO_CLKCR_CLK_DIV_CLEAR_MASK 0xFFFFFF01

    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    cyg_uint32 reg_data;

    /* Get the SDIO CLKCR value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    reg_data &= STM32_SDIO_CLKCR_CLK_DIV_CLEAR_MASK;
    reg_data |= clk_div;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

static
void stm32_sdio_set_bus_wide(cyg_uint32 bus_wide)
{
#define STM32_SDIO_CLKCR_BUS_WIDE_CLEAR_MASK ((cyg_uint32)(3 << 11))

    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    cyg_uint32 reg_data;

    /* Get the SDIO CLKCR value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    reg_data &= (~STM32_SDIO_CLKCR_BUS_WIDE_CLEAR_MASK);
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

static
void stm32_sdio_set_power_state(cyg_bool power_on)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_POWER;
    reg_data = power_on ? CYGHWR_HAL_STM32_SDIO_POWER_PWRON:
                          CYGHWR_HAL_STM32_SDIO_POWER_PWROFF;
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

static
void stm32_sdio_get_power_state(cyg_bool * power_on)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_POWER;
    cyg_uint32 reg_data;
    HAL_READ_UINT32 (reg_addr, reg_data);
    *power_on = (reg_data & CYGHWR_HAL_STM32_SDIO_POWER_PWRON) ? true : false;
}

static
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

/*
 * @argument  - Specifies the SDIO command argument which is sent to a card as
 *              part of a command message. If a command contains an argument,
 *              it must be loaded into this register before writing the command
 *              to the command register.
 * @cmd_index - Specifies the SDIO command index. It must be lower than 0x40.
 * @response  - Specifies the SDIO response type.
 *              This parameter can be a value of the follow:
 *                  CYGHWR_HAL_STM32_SDIO_CMD_NORESP_CMDSENT_0,
 *                  CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
 *                  CYGHWR_HAL_STM32_SDIO_CMD_NORESP_CMDSENT_2,
 *                  CYGHWR_HAL_STM32_SDIO_CMD_LONGRESP_CMDREND.
 * @wait      - Specifies whether SDIO wait-for-interrupt request is enabled or disabled.
 *              This parameter can be a value of the follow:
 *                  CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
 *                  CYGHWR_HAL_STM32_SDIO_CMD_WAITINT,
 *                  CYGHWR_HAL_STM32_SDIO_CMD_WAITPEND.
 * @cpsm      - Specifies whether SDIO Command path state machine (CPSM) is
 *              enabled or disabled.
 *              This parameter can be true(1) for enable or false(0) for disable.
 */
static void
stm32_sdio_send_command(cyg_uint32 argument, cyg_uint32 cmd_index,
            cyg_uint32 response, cyg_uint32 wait, cyg_bool cpsm)
{
// SDIO CMD register clear mask
#define CYGHWR_HAL_STM32_SDIO_CMD_CLEAR_MASK        ((cyg_uint32)0xFFFFF800)

    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    /* Check the parameters */
    if (!(cmd_index < 0x40) ||
        !((response == CYGHWR_HAL_STM32_SDIO_CMD_NORESP_CMDSENT_0) ||
          (response == CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND)||
          (response == CYGHWR_HAL_STM32_SDIO_CMD_NORESP_CMDSENT_2) ||
          (response == CYGHWR_HAL_STM32_SDIO_CMD_LONGRESP_CMDREND))||
        !((wait == CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE) ||
          (wait == CYGHWR_HAL_STM32_SDIO_CMD_WAITINT) ||
          (wait == CYGHWR_HAL_STM32_SDIO_CMD_WAITPEND))) {
        return;
    }

    /* Set the SDIO Argument value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_ARG;
    reg_data = argument;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    /* SDIO CMD Configuration */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CMD;
    /* Get the SDIO CMD value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    /* Clear CMDINDEX, WAITRESP, WAITINT, WAITPEND, CPSMEN bits */
    reg_data &= CYGHWR_HAL_STM32_SDIO_CMD_CLEAR_MASK;
    /* Set CMDINDEX bits according to SDIO_CmdIndex value */
    /* Set WAITRESP bits according to SDIO_Response value */
    /* Set WAITINT and WAITPEND bits according to SDIO_Wait value */
    /* Set CPSMEN bits according to SDIO_CPSM value */
    reg_data |= (cmd_index | response | wait);
    if (cpsm) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_CMD_CPSMEN;
    }

    /* Write to SDIO CMD */
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

static void
stm32_sdio_data_config(cyg_uint32 data_timeout, cyg_uint32 data_len,
   cyg_uint16 block_size, cyg_bool to_card, cyg_bool block_mode, cyg_bool dpsm_en)
{
/* SDIO DCTRL Clear Mask */
#define DCTRL_CLEAR_MASK         ((cyg_uint32)0xFFFFFF08)

    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;
    cyg_uint8 power;

    power = convert_from_bytes_to_power_of_two(block_size);

    /* Check the parameters */
    if (!(data_len <= 0x01FFFFFF) ||
        !(power >= 0 && power <= 14)) {
        DEBUG1("Invalid param @data_len = %d or @block_size = %d!\n",
                        data_len, block_size);
        return;
    }

    /*-------------------------- SDIO DTIMER Configuration -------------------*/
    /* Set the SDIO Data TimeOut value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DTIMER;
    reg_data = data_timeout;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    /*-------------------------- SDIO DLEN Configuration ---------------------*/
    /* Set the SDIO DataLength value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DLEN;
    reg_data = data_len & 0x01FFFFFF;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    /*-------------------------- SDIO DCTRL Configuration --------------------*/
    /* Get the SDIO DCTRL value */
    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DCTRL;
    HAL_READ_UINT32 (reg_addr, reg_data);
    /* Clear DEN, DTMODE, DTDIR and DBCKSIZE bits */
    reg_data &= DCTRL_CLEAR_MASK;
    /* Set DEN, DTMODE, DTDIR and DBCKSIZE bits */
    reg_data |= CYGHWR_HAL_STM32_SDIO_DCTRL_DBLOCKSIZE(power);

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

static void
stm32_sdio_data_transfer_enable(cyg_bool enable)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DCTRL;
    cyg_uint32 reg_data;
    HAL_READ_UINT32 (reg_addr, reg_data);
    if (enable) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_DCTRL_DTEN;
    } else {
        reg_data &= (~CYGHWR_HAL_STM32_SDIO_DCTRL_DTEN);
    }
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

#define STM32_SDIO_DATA_TRANSFER_ENABLE()   stm32_sdio_data_transfer_enable(1)
#define STM32_SDIO_DATA_TRANSFER_DISABLE()  stm32_sdio_data_transfer_enable(0)


/**
  * @brief  Read one data word from Rx FIFO.
  * @param  None
  * @retval Data received
  */
static
cyg_uint32 stm32_sdio_read_data(void)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_FIFO;
    cyg_uint32 reg_data;
    HAL_READ_UINT32 (reg_addr, reg_data);
    return reg_data;
}

// ----------------------------------------------------------------------------

// Checks for error conditions for CMD0.
static
stm32_sd_error_t stm32_sdio_chk_cmd0_error(void)
{
    cyg_uint32 timeout = 0x00010000; /*!< 10000 */

    while ((timeout > 0) &&
            !stm32_sdio_get_status_flag(CYGHWR_HAL_STM32_SDIO_STA_CMDSENT)) {
        timeout--;
    }

    if (timeout == 0) {
        return SD_CMD_RSP_TIMEOUT;
    }

    /*!< Clear all the static flags */
    stm32_sdio_clear_status_flag(STM32_SDIO_STA_REG_STATIC_FLAGS);
    return SD_OK;
}

// Checks for error conditions for R7 response.
static
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
static
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
static
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
static
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
static stm32_sd_error_t
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
  * @param  scr: pointer to the buffer that will contain the SCR value.
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static
stm32_sd_error_t stm32_sdio_find_scr(cyg_uint16 rca, cyg_uint32 * scr)
{
    cyg_haladdress reg_addr;
    stm32_sd_error_t errorstatus = SD_OK;
    cyg_uint32 tempscr[2] = {0, 0};
    cyg_uint32 index = 0;
    cyg_uint32 status;

    /*!< Set Block Size To 8 Bytes */
    /*!< Send CMD55 APP_CMD with argument as card's RCA */
    stm32_sdio_send_command(((cyg_uint32)8),/* Argument */
                            SD_CMD_SET_BLOCKLEN,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_SET_BLOCKLEN);
    if (errorstatus != SD_OK) {
        return errorstatus;
    }

    /*!< Send CMD55 APP_CMD with argument as card's RCA */
    stm32_sdio_send_command(((cyg_uint32) rca << 16),/* Argument */
                            SD_CMD_APP_CMD,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_CMD);
    if (errorstatus != SD_OK) {
        return errorstatus;
    }

    stm32_sdio_data_config(SD_DATATIMEOUT, 8, 8, 0, 1, 1);

    // 解决偶尔读取SCR失败的问题(尽管机率很小，但调试的时候还是有发现)，最重要的
    // 是使读取的SCR值与实际值相符。未有该延时之前，读取到的SCR为:
    // tempscr[0]= 0x00802502, tempscr[1]= 0x00000000,
    // 实际值应该是: should be 0x00802502,0x00000001
    hal_delay_us(50);

    /*!< Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
    DEBUG1("Send ACMD51 SD_APP_SEND_SCR with argument as 0x00\n");
    stm32_sdio_send_command(0x00,/* Argument */
                            SD_CMD_SD_APP_SEND_SCR,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_SD_APP_SEND_SCR);
    if (errorstatus != SD_OK) {
        return errorstatus;
    }

    reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    HAL_READ_UINT32 (reg_addr, status);
    while (!(status & (CYGHWR_HAL_STM32_SDIO_STA_RXOVERR |
        CYGHWR_HAL_STM32_SDIO_STA_DCRCFAIL | CYGHWR_HAL_STM32_SDIO_STA_DTIMEOUT |
        CYGHWR_HAL_STM32_SDIO_STA_DBCKEND | CYGHWR_HAL_STM32_SDIO_STA_STBITERR))) {
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

    DEBUG1("tempscr[0]= 0x%08X, tempscr[1]= 0x%08X, should be 0x00802502,0x00000001\n",
                    tempscr[0], tempscr[1]);

    *(scr + 1) = ((tempscr[0] & SD_0TO7BITS) << 24) |
                  ((tempscr[0] & SD_8TO15BITS) << 8) |
                  ((tempscr[0] & SD_16TO23BITS) >> 8) |
                  ((tempscr[0] & SD_24TO31BITS) >> 24);

    *(scr) = ((tempscr[1] & SD_0TO7BITS) << 24) |
              ((tempscr[1] & SD_8TO15BITS) << 8) |
              ((tempscr[1] & SD_16TO23BITS) >> 8) |
              ((tempscr[1] & SD_24TO31BITS) >> 24);

    return  SD_OK;
}

/**
  * @brief  Enables or disables the SDIO DMA request.
  * @param  enable: new state of the selected SDIO DMA request.
  *   This parameter can be: true: ENABLE or false: DISABLE.
  * @retval None
  */
static
void stm32_sdio_dma_enable(cyg_bool enable)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_DCTRL;
    cyg_uint32 reg_data;

    /* Get the SDIO CLKCR value */
    HAL_READ_UINT32 (reg_addr, reg_data);
    if (enable) {
        reg_data |= CYGHWR_HAL_STM32_SDIO_DCTRL_DMAEN;
    } else {
        reg_data &= (~CYGHWR_HAL_STM32_SDIO_DCTRL_DMAEN);
    }
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

#define STM32_SDIO_DMA_ENABLE()     stm32_sdio_dma_enable(1)
#define STM32_SDIO_DMA_DISABLE()    stm32_sdio_dma_enable(0)

/**
  * @brief  Enables or disables the SDIO hardware flow control.
  *
  * SDIO hardware flow control must be enabled when transfer datas with DMA.
  * See http://www.eefocus.com/bbs/article_244_189560.html
  */
static
void stm32_sdio_hw_flow_control_enable(cyg_bool enable)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_CLKCR;
    cyg_uint32 reg_data;

    HAL_READ_UINT32 (reg_addr, reg_data);

    if (enable) {
        /* Enable hardware flow control */
        reg_data |= CYGHWR_HAL_STM32_SDIO_CLKCR_HWFC_EN;
    } else {
        reg_data &= (~CYGHWR_HAL_STM32_SDIO_CLKCR_HWFC_EN);
    }

    HAL_WRITE_UINT32 (reg_addr, reg_data);
}

#define STM32_SDIO_HW_FLOW_CONTROL_ENABLE()  stm32_sdio_hw_flow_control_enable(1)
#define STM32_SDIO_HW_FLOW_CONTROL_DISABLE() stm32_sdio_hw_flow_control_enable(0)


// ----------------------------------------------------------------------------
// -------------------------- SD/MMC Operation --------------------------------
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------

static stm32_sd_error_t
stm32_mmc_get_card_cid(cyg_uint32 cid_cmd_index, cyg_uint32 cid[4])
{
    stm32_sd_error_t errorstatus = SD_OK;

    if (!(cid_cmd_index == SD_CMD_ALL_SEND_CID ||
          cid_cmd_index == SD_CMD_SEND_CID)) {
        DEBUG1("Invalid CID command: CMD%d\n", cid_cmd_index);
        return  SD_INVALID_PARAMETER;
    }

    /*!< Send CMD2 ALL_SEND_CID */
    stm32_sdio_send_command(0x00,/* Argument */
                            cid_cmd_index,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_LONGRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    if ((errorstatus = stm32_sdio_chk_cmd_resp2_error()) !=  SD_OK) {
        return errorstatus;
    }

    cid[0] = stm32_sdio_get_response(1);
    cid[1] = stm32_sdio_get_response(2);
    cid[2] = stm32_sdio_get_response(3);
    cid[3] = stm32_sdio_get_response(4);

    DEBUG1("MMC/SD card CID = %08X_%08X_%08X_%08X\n", cid[3], cid[2], cid[1], cid[0]);

    return  errorstatus;
}

static stm32_sd_error_t
stm32_mmc_get_card_csd(cyg_uint16 rca, cyg_uint32 csd[4])
{
    stm32_sd_error_t errorstatus = SD_OK;

    /*!< Send CMD9 SEND_CSD with argument as card's RCA */
    stm32_sdio_send_command(((cyg_uint32)(rca << 16)),/* Argument */
                            SD_CMD_SEND_CSD,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_LONGRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    if ((errorstatus = stm32_sdio_chk_cmd_resp2_error()) != SD_OK) {
        return errorstatus;
    }

    csd[0] = stm32_sdio_get_response(1);
    csd[1] = stm32_sdio_get_response(2);
    csd[2] = stm32_sdio_get_response(3);
    csd[3] = stm32_sdio_get_response(4);

    DEBUG1("MMC/SD card CSD = %08X_%08X_%08X_%08X\n", csd[3], csd[2], csd[1], csd[0]);

    return  errorstatus;
}

/**
  * @brief  Enquires cards about their operating voltage and configures
  *   clock controls.
  * @param  None
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_power_on(cyg_mmc_stm32_disk_info_t * disk)
{
#define SD_VOLTAGE_WINDOW_SD            ((cyg_uint32)0x80100000)
#define SD_HIGH_CAPACITY                ((cyg_uint32)0x40000000)
#define SD_STD_CAPACITY                 ((cyg_uint32)0x00000000)
#define SD_CHECK_PATTERN                ((cyg_uint32)0x000001AA)
#define SD_MAX_VOLT_TRIAL               ((cyg_uint32)0x0000FFFF)
/**
  * @brief  Following commands are SD Card Specific commands.
  *         SDIO_APP_CMD should be sent before sending these commands.
  */
#define SDIO_SEND_IF_COND               ((cyg_uint32)0x00000008)

    stm32_sd_error_t errorstatus = SD_OK;
    cyg_uint32 sd_type = SD_STD_CAPACITY;
    cyg_uint32 response = 0, count = 0, validvoltage = 0;

    /*!< Power ON Sequence ---------------------------------------------------*/
    /*!< Configure the SDIO peripheral */
    /*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_INIT_CLK_DIV) */
    /*!< SDIO_CK for initialization should not exceed 400 KHz */
    stm32_sdio_init();

    /*!< Set Power State to ON */
    stm32_sdio_set_power_state(true);

    /*!< Enable SDIO Clock */
    stm32_sdio_clk_enable(true);

    /*!< CMD0: GO_IDLE_STATE -------------------------------------------------*/
    /*!< No CMD response required */
    int i;
    // 配置的时候，sd卡需要至少发74个时钟使它自己初始化，这是2.0规范要求的
    for (i = 0; i < 74; i++) {
        stm32_sdio_send_command(0x00,/* Argument */
                                SD_CMD_GO_IDLE_STATE,/* Command */
                                CYGHWR_HAL_STM32_SDIO_CMD_NORESP_CMDSENT_0,
                                CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                true);
    }
    if ((errorstatus = stm32_sdio_chk_cmd0_error()) != SD_OK) {
        return errorstatus;
    }

    /*!< CMD8: SEND_IF_COND --------------------------------------------------*/
    /*!< Send CMD8 to verify SD card interface operating condition */
    /*!< Argument: - [31:12]: Reserved (shall be set to '0')
               - [11:8]: Supply Voltage (VHS) 0x1 (Range: 2.7-3.6 V)
               - [7:0]: Check Pattern (recommended 0xAA) */
    /*!< CMD Response: R7 */
    stm32_sdio_send_command(0x000001AA,/* Argument */
                            SDIO_SEND_IF_COND,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    if ((errorstatus = stm32_sdio_chk_cmd_resp7_error()) == SD_OK) {
        disk->card_type = SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V2_0; /*!< SD Card 2.0 */
        sd_type = SD_HIGH_CAPACITY;
        DEBUG1("SD Card 2.0\n");
    } else ;/*!< CMD55 */

    /*!< CMD55 */
    stm32_sdio_send_command(0x00,/* Argument */
                            SD_CMD_APP_CMD,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_CMD);

    /*!< If errorstatus is Command TimeOut, it is a MMC card */
    /*!< If errorstatus is SD_OK it is a SD card: SD card 2.0 (voltage range mismatch)
         or SD card 1.x */
    if (errorstatus == SD_OK) {
        /*!< SD CARD */
        /*!< Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
        while ((!validvoltage) && (count < SD_MAX_VOLT_TRIAL)) {

            /*!< SEND CMD55 APP_CMD with RCA as 0 */
            stm32_sdio_send_command(0x00,/* Argument */
                                    SD_CMD_APP_CMD,/* Command */
                                    CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                    CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                    true);
            errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_CMD);
            if (errorstatus != SD_OK) {
                #if 1
        			/* 2011-10-21 部分国产2G SD卡,不支持这个命令，直接返回OK
        			这样修改后，并不影响SDHC卡类型的判断
        			*/
        			return SD_OK;
        		#else
        			return(errorstatus); 	/* 这是ST原来的代码 */
        		#endif
            }

            stm32_sdio_send_command((SD_VOLTAGE_WINDOW_SD | sd_type),/* Argument */
                                    SD_CMD_SD_APP_OP_COND,/* Command */
                                    CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                    CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                    true);
            if ((errorstatus = stm32_sdio_chk_cmd_resp3_error()) != SD_OK) {
                return  errorstatus;
            }

            response = stm32_sdio_get_response(1);
            validvoltage = (((response >> 31) == 1) ? 1 : 0);
            count++;
        }

        DEBUG1("count = %d\n", count);
        if (count >= SD_MAX_VOLT_TRIAL) {
            return  SD_INVALID_VOLTRANGE;
        }

        if (response &= SD_HIGH_CAPACITY) {
            disk->card_type = SD_CARD_TYPE_HIGH_CAPACITY_SD_CARD;
        }
    } /*!< else MMC Card */

    return  errorstatus;
}

/**
  * @brief  Intialises all cards or single card as the case may be Card(s) come
  *         into standby state.
  * @param  None
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_initialize_cards(cyg_mmc_stm32_disk_info_t * disk)
{
    stm32_sd_error_t errorstatus = SD_OK;
    cyg_uint16 rca = 0x01;
    cyg_bool flag;

    stm32_sdio_get_power_state(&flag);
    if (!flag) {
        return  SD_REQUEST_NOT_APPLICABLE;
    }

    if (SD_CARD_TYPE_SECURE_DIGITAL_IO_CARD != disk->card_type) {
        errorstatus = stm32_mmc_get_card_cid(SD_CMD_ALL_SEND_CID, CID_Tab);
        if (errorstatus != SD_OK) {
            return errorstatus;
        }
    }

    if ((SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V1_1 == disk->card_type) ||
        (SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V2_0 == disk->card_type) ||
        (SD_CARD_TYPE_SECURE_DIGITAL_IO_COMBO_CARD == disk->card_type) ||
        (SD_CARD_TYPE_HIGH_CAPACITY_SD_CARD == disk->card_type))
    {
        /*!< Send CMD3 SET_REL_ADDR with argument 0 */
        /*!< SD Card publishes its RCA. */
        stm32_sdio_send_command(0x00,/* Argument */
                                SD_CMD_SET_REL_ADDR,/* Command */
                                CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                true);
        errorstatus = stm32_sdio_chk_cmd_resp6_error(SD_CMD_SET_REL_ADDR, &rca);
        if (SD_OK != errorstatus) {
            return errorstatus;
        }
        DEBUG1("MMC/SD card RCA = 0x%04X\n", rca);
    }

    if (SD_CARD_TYPE_SECURE_DIGITAL_IO_CARD != disk->card_type) {
        disk->rca = rca;
        if ((errorstatus = stm32_mmc_get_card_csd(rca, CSD_Tab)) != SD_OK) {
            return errorstatus;
        }
    }

    /*!< All cards get intialized */
    return SD_OK;
}

/**
  * @brief  Returns information about specific card.
  * @param  cardinfo: pointer to a SD_CardInfo structure that contains all SD card
  *         information.
  * @retval SD_Error: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_get_card_info(cyg_mmc_stm32_disk_info_t * disk)
{
    cyg_uint8 tmp = 0;

    /*!< Byte 0 */
    tmp = (cyg_uint8)((CSD_Tab[0] & 0xFF000000) >> 24);
    disk->csd.CSDStruct = (tmp & 0xC0) >> 6;
    disk->csd.SysSpecVersion = (tmp & 0x3C) >> 2;
    disk->csd.Reserved1 = tmp & 0x03;

    /*!< Byte 1 */
    tmp = (cyg_uint8)((CSD_Tab[0] & 0x00FF0000) >> 16);
    disk->csd.TAAC = tmp;

    /*!< Byte 2 */
    tmp = (cyg_uint8)((CSD_Tab[0] & 0x0000FF00) >> 8);
    disk->csd.NSAC = tmp;

    /*!< Byte 3 */
    tmp = (cyg_uint8)(CSD_Tab[0] & 0x000000FF);
    disk->csd.MaxBusClkFrec = tmp;

    /*!< Byte 4 */
    tmp = (cyg_uint8)((CSD_Tab[1] & 0xFF000000) >> 24);
    disk->csd.CardComdClasses = tmp << 4;

    /*!< Byte 5 */
    tmp = (cyg_uint8)((CSD_Tab[1] & 0x00FF0000) >> 16);
    disk->csd.CardComdClasses |= (tmp & 0xF0) >> 4;
    disk->csd.RdBlockLen = tmp & 0x0F;

    /*!< Byte 6 */
    tmp = (cyg_uint8)((CSD_Tab[1] & 0x0000FF00) >> 8);
    disk->csd.PartBlockRead = (tmp & 0x80) >> 7;
    disk->csd.WrBlockMisalign = (tmp & 0x40) >> 6;
    disk->csd.RdBlockMisalign = (tmp & 0x20) >> 5;
    disk->csd.DSRImpl = (tmp & 0x10) >> 4;
    disk->csd.Reserved2 = 0; /*!< Reserved */

    if ((disk->card_type == SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V1_1) ||
        (disk->card_type == SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V2_0)) {
        disk->csd.DeviceSize = (tmp & 0x03) << 10;

        /*!< Byte 7 */
        tmp = (cyg_uint8)(CSD_Tab[1] & 0x000000FF);
        disk->csd.DeviceSize |= (tmp) << 2;

        /*!< Byte 8 */
        tmp = (cyg_uint8)((CSD_Tab[2] & 0xFF000000) >> 24);
        disk->csd.DeviceSize |= (tmp & 0xC0) >> 6;

        disk->csd.MaxRdCurrentVDDMin = (tmp & 0x38) >> 3;
        disk->csd.MaxRdCurrentVDDMax = (tmp & 0x07);

        /*!< Byte 9 */
        tmp = (cyg_uint8)((CSD_Tab[2] & 0x00FF0000) >> 16);
        disk->csd.MaxWrCurrentVDDMin = (tmp & 0xE0) >> 5;
        disk->csd.MaxWrCurrentVDDMax = (tmp & 0x1C) >> 2;
        disk->csd.DeviceSizeMul = (tmp & 0x03) << 1;
        /*!< Byte 10 */
        tmp = (cyg_uint8)((CSD_Tab[2] & 0x0000FF00) >> 8);
        disk->csd.DeviceSizeMul |= (tmp & 0x80) >> 7;

        disk->capacity = (disk->csd.DeviceSize + 1) ;
        disk->capacity *= (1 << (disk->csd.DeviceSizeMul + 2));
        disk->blocksize = 1 << (disk->csd.RdBlockLen);
        disk->capacity *= disk->blocksize;
    }
    else if (disk->card_type == SD_CARD_TYPE_HIGH_CAPACITY_SD_CARD) {
        /*!< Byte 7 */
        tmp = (cyg_uint8)(CSD_Tab[1] & 0x000000FF);
        disk->csd.DeviceSize = (tmp & 0x3F) << 16;

        /*!< Byte 8 */
        tmp = (cyg_uint8)((CSD_Tab[2] & 0xFF000000) >> 24);

        disk->csd.DeviceSize |= (tmp << 8);

        /*!< Byte 9 */
        tmp = (cyg_uint8)((CSD_Tab[2] & 0x00FF0000) >> 16);

        disk->csd.DeviceSize |= (tmp);

        /*!< Byte 10 */
        tmp = (cyg_uint8)((CSD_Tab[2] & 0x0000FF00) >> 8);

        disk->capacity  = (disk->csd.DeviceSize + 1) * 512 * 1024;
        disk->blocksize = 512;
    }


    disk->csd.EraseGrSize = (tmp & 0x40) >> 6;
    disk->csd.EraseGrMul = (tmp & 0x3F) << 1;

    /*!< Byte 11 */
    tmp = (cyg_uint8)(CSD_Tab[2] & 0x000000FF);
    disk->csd.EraseGrMul |= (tmp & 0x80) >> 7;
    disk->csd.WrProtectGrSize = (tmp & 0x7F);

    /*!< Byte 12 */
    tmp = (cyg_uint8)((CSD_Tab[3] & 0xFF000000) >> 24);
    disk->csd.WrProtectGrEnable = (tmp & 0x80) >> 7;
    disk->csd.ManDeflECC = (tmp & 0x60) >> 5;
    disk->csd.WrSpeedFact = (tmp & 0x1C) >> 2;
    disk->csd.MaxWrBlockLen = (tmp & 0x03) << 2;

    /*!< Byte 13 */
    tmp = (cyg_uint8)((CSD_Tab[3] & 0x00FF0000) >> 16);
    disk->csd.MaxWrBlockLen |= (tmp & 0xC0) >> 6;
    disk->csd.WriteBlockPaPartial = (tmp & 0x20) >> 5;
    disk->csd.Reserved3 = 0;
    disk->csd.ContentProtectAppli = (tmp & 0x01);

    /*!< Byte 14 */
    tmp = (cyg_uint8)((CSD_Tab[3] & 0x0000FF00) >> 8);
    disk->csd.FileFormatGrouop = (tmp & 0x80) >> 7;
    disk->csd.CopyFlag = (tmp & 0x40) >> 6;
    disk->csd.PermWrProtect = (tmp & 0x20) >> 5;
    disk->csd.TempWrProtect = (tmp & 0x10) >> 4;
    disk->csd.FileFormat = (tmp & 0x0C) >> 2;
    disk->csd.ECC = (tmp & 0x03);

    /*!< Byte 15 */
    tmp = (cyg_uint8)(CSD_Tab[3] & 0x000000FF);
    disk->csd.CSD_CRC = (tmp & 0xFE) >> 1;
    disk->csd.Reserved4 = 1;


    /*!< Byte 0 */
    tmp = (cyg_uint8)((CID_Tab[0] & 0xFF000000) >> 24);
    disk->cid.ManufacturerID = tmp;

    /*!< Byte 1 */
    tmp = (cyg_uint8)((CID_Tab[0] & 0x00FF0000) >> 16);
    disk->cid.OEM_AppliID = tmp << 8;

    /*!< Byte 2 */
    tmp = (cyg_uint8)((CID_Tab[0] & 0x000000FF00) >> 8);
    disk->cid.OEM_AppliID |= tmp;

    /*!< Byte 3 */
    tmp = (cyg_uint8)(CID_Tab[0] & 0x000000FF);
    disk->cid.ProdName1 = tmp << 24;

    /*!< Byte 4 */
    tmp = (cyg_uint8)((CID_Tab[1] & 0xFF000000) >> 24);
    disk->cid.ProdName1 |= tmp << 16;

    /*!< Byte 5 */
    tmp = (cyg_uint8)((CID_Tab[1] & 0x00FF0000) >> 16);
    disk->cid.ProdName1 |= tmp << 8;

    /*!< Byte 6 */
    tmp = (cyg_uint8)((CID_Tab[1] & 0x0000FF00) >> 8);
    disk->cid.ProdName1 |= tmp;

    /*!< Byte 7 */
    tmp = (cyg_uint8)(CID_Tab[1] & 0x000000FF);
    disk->cid.ProdName2 = tmp;

    /*!< Byte 8 */
    tmp = (cyg_uint8)((CID_Tab[2] & 0xFF000000) >> 24);
    disk->cid.ProdRev = tmp;

    /*!< Byte 9 */
    tmp = (cyg_uint8)((CID_Tab[2] & 0x00FF0000) >> 16);
    disk->cid.ProdSN = tmp << 24;

    /*!< Byte 10 */
    tmp = (cyg_uint8)((CID_Tab[2] & 0x0000FF00) >> 8);
    disk->cid.ProdSN |= tmp << 16;

    /*!< Byte 11 */
    tmp = (cyg_uint8)(CID_Tab[2] & 0x000000FF);
    disk->cid.ProdSN |= tmp << 8;

    /*!< Byte 12 */
    tmp = (cyg_uint8)((CID_Tab[3] & 0xFF000000) >> 24);
    disk->cid.ProdSN |= tmp;

    /*!< Byte 13 */
    tmp = (cyg_uint8)((CID_Tab[3] & 0x00FF0000) >> 16);
    disk->cid.Reserved1 |= (tmp & 0xF0) >> 4;
    disk->cid.ManufactDate = (tmp & 0x0F) << 8;

    /*!< Byte 14 */
    tmp = (cyg_uint8)((CID_Tab[3] & 0x0000FF00) >> 8);
    disk->cid.ManufactDate |= tmp;

    /*!< Byte 15 */
    tmp = (cyg_uint8)(CID_Tab[3] & 0x000000FF);
    disk->cid.CID_CRC = (tmp & 0xFE) >> 1;
    disk->cid.Reserved2 = 1;

    return SD_OK;
}

/**
  * @brief  Selects od Deselects the corresponding card.
  * @param  rca: RCA of the Card to be selected.
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_select_deselect_card(cyg_uint16 rca)
{
    DEBUG1("Send CMD7 SDIO_SEL_DESEL_CARD, arg = 0x%08X\n", ((cyg_uint32)(rca << 16)));

    /*!< Send CMD7 SDIO_SEL_DESEL_CARD */
    stm32_sdio_send_command(((cyg_uint32)(rca << 16)),/* Argument */
                            SD_CMD_SEL_DESEL_CARD,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    return stm32_sdio_chk_cmd_resp1_error(SD_CMD_SEL_DESEL_CARD);
}

/**
  * @brief  Enables or disables the SDIO wide bus mode.
  * @param  enable:
  *   This parameter can be: true for enable or false of disable.
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_enable_card_widebus(cyg_mmc_stm32_disk_info_t * disk,
                                cyg_bool enable)
{
    stm32_sd_error_t errorstatus = SD_OK;
    cyg_uint32 scr[2] = {0, 0};

    if (stm32_sdio_get_response(1) & SD_CARD_LOCKED) {
        return SD_LOCK_UNLOCK_FAILED;
    }

    /*!< Get SCR Register */
    if ((errorstatus = stm32_sdio_find_scr(disk->rca, scr)) != SD_OK) {
        return errorstatus;
    }
    DEBUG1("SD/MMC card SCR = %08X_%08X (Hex), should be 02258000_01000000\n", scr[1], scr[0]);

    /*!< If wide bus operation to be enabled */
    if (enable) {
        /*!< If requested card supports wide bus operation */
        if ((scr[1] & SD_WIDE_BUS_SUPPORT) != SD_ALLZERO) {
            DEBUG1("Requested card supports wide bus operation\n");

            /*!< Send CMD55 APP_CMD with argument as card's RCA.*/
            stm32_sdio_send_command(((cyg_uint32)(disk->rca << 16)),/* Argument */
                                    SD_CMD_APP_CMD,/* Command */
                                    CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                    CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                    true);
            errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_CMD);
            if (errorstatus != SD_OK) {
                return errorstatus;
            }


            /*!< Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
            stm32_sdio_send_command(0x02,/* Argument */
                                    SD_CMD_APP_SD_SET_BUSWIDTH,/* Command */
                                    CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                    CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                    true);
            return stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_SD_SET_BUSWIDTH);
        } else {
            DEBUG1("Requested card don't support wide bus operation\n");
            return SD_REQUEST_NOT_APPLICABLE;
        }
    }

    /*!< If wide bus operation to be disabled */
    else {
        /*!< If requested card supports 1 bit mode operation */
        if ((scr[1] & SD_SINGLE_BUS_SUPPORT) != SD_ALLZERO) {
            DEBUG1("Requested card supports 1 bit mode operation\n");

            /*!< Send CMD55 APP_CMD with argument as card's RCA.*/
            stm32_sdio_send_command(((cyg_uint32)(disk->rca << 16)),/* Argument */
                                    SD_CMD_APP_CMD,/* Command */
                                    CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                    CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                    true);
            errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_CMD);
            if (errorstatus != SD_OK) {
                return errorstatus;
            }

            /*!< Send ACMD6 APP_CMD with argument as 0 for wide bus mode */
            stm32_sdio_send_command(0x00,/* Argument */
                                    SD_CMD_APP_SD_SET_BUSWIDTH,/* Command */
                                    CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                    CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                    true);
            return stm32_sdio_chk_cmd_resp1_error(SD_CMD_APP_SD_SET_BUSWIDTH);
        } else {
            DEBUG1("Requested card don't support 1 bit mode operation\n");
            return SD_REQUEST_NOT_APPLICABLE;
        }
    }

    return(errorstatus);
}

/**
  * @brief  Enables wide bus opeartion for the requeseted card if supported by
  *         card.
  * @param  wide_mode: Specifies the SD card wide bus mode.
  *   This parameter can be one of the following values:
  *     @arg SDIO_BusWide_8b: 8-bit data transfer (Only for MMC)
  *     @arg SDIO_BusWide_4b: 4-bit data transfer
  *     @arg SDIO_BusWide_1b: 1-bit data transfer
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_enable_widebus_operation(cyg_mmc_stm32_disk_info_t * disk,
                                cyg_uint32 wide_mode)
{
    stm32_sd_error_t errorstatus = SD_OK;

    /*!< MMC Card doesn't support this feature */
    if (SD_CARD_TYPE_MULTIMEDIA_CARD == disk->card_type) {
        return SD_UNSUPPORTED_FEATURE;
    } else if ((SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V1_1 == disk->card_type) ||
               (SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V2_0 == disk->card_type) ||
               (SD_CARD_TYPE_HIGH_CAPACITY_SD_CARD == disk->card_type)) {

        if (SDIO_BusWide_8b == wide_mode) {
            return SD_UNSUPPORTED_FEATURE;
        } else if (SDIO_BusWide_4b == wide_mode) {
            errorstatus = stm32_mmc_enable_card_widebus(disk, true);
        } else {
            errorstatus = stm32_mmc_enable_card_widebus(disk, false);
        }

        if (SD_OK == errorstatus) {
            /*!< Configure the SDIO peripheral */
            DEBUG1("Configure the SDIO bus to %d bit(s)\n",
                        SDIO_BusWide_4b == wide_mode ? 4 : 1);
            stm32_sdio_set_bus_wide(wide_mode);
        }
    }

    return(errorstatus);
}

/**
  * @brief  Sets device mode whether to operate in Polling, Interrupt or DMA mode.
  * @param  Mode: Specifies the Data Transfer mode.
  *   This parameter can be one of the following values:
  *     @arg SD_DMA_MODE: Data transfer using DMA.
  *     @arg SD_INTERRUPT_MODE: Data transfer using interrupts.
  *     @arg SD_POLLING_MODE: Data transfer using flags.
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_set_device_mode(cyg_uint32 mode)
{
  stm32_sd_error_t errorstatus = SD_OK;
#if 0
    if ((mode == SD_DMA_MODE) || (mode == SD_INTERRUPT_MODE) ||
        (mode == SD_POLLING_MODE)) {
//      DeviceMode = mode;
    } else {
        errorstatus = SD_INVALID_PARAMETER;
    }
#else
//  STM32_SDIO_DMA_ENABLE();
#endif
    return(errorstatus);
}

/**
  * @brief  Initializes the SD Card and put it into StandBy State (Ready for data
  *         transfer).
  * @param  None
  * @retval stm32_sd_error_t: SD Card Error code.
  */
static stm32_sd_error_t
stm32_mmc_init(cyg_mmc_stm32_disk_info_t * disk)
{
    stm32_sd_error_t errorstatus = SD_OK;

    stm32_sdio_deinit();

    errorstatus = stm32_mmc_power_on(disk);
    if (errorstatus != SD_OK) {
        /*!< CMD Response TimeOut (wait for CMDSENT flag) */
        return  errorstatus;
    }

    errorstatus = stm32_mmc_initialize_cards(disk);
    if (errorstatus != SD_OK) {
        /*!< CMD Response TimeOut (wait for CMDSENT flag) */
        return  errorstatus;
    }

    /*!< Configure the SDIO peripheral */
    /*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_TRANSFER_CLK_DIV) */
    stm32_sdio_set_clk_div(STM32_SDIO_TRANSFER_CLK_DIV_18MHz);

    /*----------------- Read CSD/CID MSD registers ------------------*/
    if (errorstatus == SD_OK) {
        errorstatus = stm32_mmc_get_card_info(disk);
    }

    /*----------------- Select Card --------------------------------*/
    if (errorstatus == SD_OK) {
        errorstatus = stm32_mmc_select_deselect_card(disk->rca);
    }

    if (errorstatus == SD_OK) {
        errorstatus = stm32_mmc_enable_widebus_operation(disk, SDIO_BusWide_4b);
    }

    /* Set Device Transfer Mode to DMA */
    if (errorstatus == SD_OK) {
        errorstatus = stm32_mmc_set_device_mode(SD_DMA_MODE);
    }

    /**
      * SDIO hardware flow control must be enabled when transfer datas with DMA.
      * See http://www.eefocus.com/bbs/article_244_189560.html
      */
    STM32_SDIO_HW_FLOW_CONTROL_ENABLE();

    return errorstatus;
}

// Detect if SD/MMC card is correctly plugged in the memory slot.
// Return true: the card is plugged.
static cyg_bool
stm32_mmc_card_detect(cyg_mmc_stm32_disk_info_t * disk)
{
    int val;

    /*!< Check GPIO to detect SD/MMC card */
    /* H-level: not plugged */
    CYGHWR_HAL_STM32_GPIO_IN(STM32_MMC_CARD_DETECT_PIN, (&val));
    DEBUG1("SD/MMC card is %s in the memory slot.\n", val ? "plugged" : "not plugged");
    return  (val ? true : false);
}


// ----------------------------------------------------------------------------
// --------------------------- DMA Operation ----------------------------------
// ----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Set up a DMA channel.

static void
stm32_dma_channel_setup(cyg_mmc_stm32_disk_info_t * disk,
                   cyg_uint8 * data_buf, cyg_uint32 count, cyg_bool polled)
{
    hal_stm32_dma_stream * stream;

    stream = disk->is_tx ? &disk->dma_tx_stream : &disk->dma_rx_stream;
    hal_stm32_dma_configure( stream,
                             32,
                             (data_buf == NULL),
                             polled );

    if( data_buf == NULL )
        data_buf = (cyg_uint8 *)&dma_null;

    hal_stm32_dma_start( stream,
                         data_buf,
                         disk->setup->sdio_reg_base + CYGHWR_HAL_STM32_SDIO_FIFO,
                         (count >> 2) );

//  hal_stm32_dma_show( stream );
}


//-----------------------------------------------------------------------------
// DMA Callbacks. The DMA driver has disabled the DMA channel and
// masked the interrupt condition, here we need to wake up the client
// thread.

static void
stm32_dma_callback( hal_stm32_dma_stream *stream,
                          cyg_uint32 count, CYG_ADDRWORD data )
{
    cyg_mmc_stm32_disk_info_t * disk = (cyg_mmc_stm32_disk_info_t *) data;

    cyg_drv_dsr_lock ();
    disk->dma_done = true;
    cyg_drv_cond_signal (&disk->condvar);
    cyg_drv_dsr_unlock ();
}


//-----------------------------------------------------------------------------
// Initiate a DMA transfer.

static Cyg_ErrNo
stm32_dma_transaction_begin(cyg_mmc_stm32_disk_info_t * disk,
           cyg_bool polled, cyg_bool is_tx, cyg_uint8 * data, cyg_uint32 count)
{
    cyg_uint8 * local_data = NULL;

    DEBUG1("Transaction begin...count = %d, polled = %d, %s\n",
                    count, polled, is_tx ? "write" : "read");

    // We check that the buffers are word aligned and that count is a
    // multiple of 4 in order to carry out the 32-bit transfer.
    CYG_ASSERT (!(count & 1) && !((cyg_uint32)data & 1),
                           "STM32 SDIO : Misaligned data in 32-bit transfer.");
    // Check for unsupported transactions.
    CYG_ASSERT (count > 0, "STM32 SDIO : Null transfer requested.");


    // Perform transfer via the bounce buffers.
    if (disk->setup->bbuf_size != 0) {
        // If the requested transfer is too large for the bounce buffer we assert
        // in debug builds and truncate in production builds.
        if (count > disk->setup->bbuf_size) {
            CYG_ASSERT(false, "STM32 SDIO : Transfer exceeds bounce buffer size.");
            count = disk->setup->bbuf_size;
        }

        local_data = disk->setup->bbuf;
        if (is_tx) {
            memcpy (local_data, data, count);
        }
    } else {
        local_data = data;
    }

    // Set runtime data
    disk->is_tx     = is_tx;
    disk->buf       = data;
    disk->count     = count;
    disk->polled    = polled;
    disk->dma_done  = false;// Clear done flags prior to configuring DMA

    // Set up the DMA channels.
    stm32_dma_channel_setup(disk, local_data, count, polled);

    return ENOERR;
}

static int
stm32_dma_check_transfer_status(cyg_mmc_stm32_disk_info_t * disk)
{
    cyg_haladdress reg_addr = CYGHWR_HAL_STM32_SDIO + CYGHWR_HAL_STM32_SDIO_STA;
    cyg_uint32 reg_data = 0;
    int status = 0;

    HAL_READ_UINT32 (reg_addr, reg_data);
    DEBUG1("STM32 SDIO STA Reg = 0x%08X\n", reg_data);

    if (reg_data & CYGHWR_HAL_STM32_SDIO_STA_DATAEND) {
        DEBUG1("STM32 SDIO DMA transfer DATAEND(Data end "
               "(data counter, SDIO_DCOUNT, is zero))\n");
        status = 1;
    }
    if (reg_data & CYGHWR_HAL_STM32_SDIO_STA_DCRCFAIL) {
        DEBUG1("STM32 SDIO DMA transfer DCRCFAIL\n");
        status = -1;
    }
    if (reg_data & CYGHWR_HAL_STM32_SDIO_STA_DTIMEOUT) {
        DEBUG1("STM32 SDIO DMA transfer DTIMEOUT(Data timeout)\n");
        status = -1;
    }
    if (reg_data & CYGHWR_HAL_STM32_SDIO_STA_RXOVERR) {
        DEBUG1("STM32 SDIO DMA transfer RXOVERR(Received FIFIO overrun error)\n");
        status = -1;
    }
    if (reg_data & CYGHWR_HAL_STM32_SDIO_STA_TXUNDERR) {
        DEBUG1("STM32 SDIO DMA transfer TXUNDERR(Transmit FIFIO underrun error)\n");
        status = -1;
    }
    if (reg_data & CYGHWR_HAL_STM32_SDIO_STA_STBITERR) {
        DEBUG1("STM32 SDIO DMA transfer STBITERR(Start bit not "
               "detected on all data signals in wide bus mode)\n");
        status = -1;
    }

    if (status < 0) {
        DEBUG1("STM32 SDIO : Transfer error! status = 0x%08X\n", reg_data);
    }
    if (status > 0) {
        DEBUG1("STM32 SDIO DMA transfer(%s) OK!\n", disk->is_tx ? "write" : "read");
    }

    return  status;
}

static void
stm32_dma_transaction(cyg_mmc_stm32_disk_info_t * disk)
{
    cyg_uint32 timeout = SD_DATATIMEOUT;
    hal_stm32_dma_stream * stream = disk->is_tx ?
                    &disk->dma_tx_stream : &disk->dma_rx_stream;

    // Enable STM32 SDIO DMA.
    STM32_SDIO_DMA_ENABLE();

    // Run the DMA (polling for completion).
    if (disk->polled) {
        while( !(disk->dma_done) && timeout > 0) {
            hal_stm32_dma_poll( stream );

#if 0 // 如果在这里读STA寄存器，则数据传输出错，导致死循环
            if (stm32_dma_check_transfer_status(disk)) {
                break;
            }
#endif

            timeout--;
        }
    }
    // Run the DMA (interrupt driven).
    else {
        cyg_drv_mutex_lock (&disk->mutex);
        cyg_drv_dsr_lock ();

        // Sit back and wait for the ISR/DSRs to signal completion.
        do {
            cyg_drv_cond_wait (&disk->condvar);
        } while (!(disk->dma_done) );

        cyg_drv_dsr_unlock ();
        cyg_drv_mutex_unlock (&disk->mutex);
    }

    // Disable STM32 SDIO DMA.
    STM32_SDIO_DMA_DISABLE();

    if (timeout == 0) {
        DEBUG1("STM32 SDIO DMA transfer timeout!\n");
        stm32_dma_check_transfer_status(disk);
    }
}

static Cyg_ErrNo
stm32_dma_transaction_end(cyg_mmc_stm32_disk_info_t * disk)
{
    if (disk->dma_done && disk->setup->bbuf_size != 0) {
        if (!disk->is_tx) {
            memcpy (disk->buf, disk->setup->bbuf, disk->count);
        }
    }

    DEBUG1("Transaction end.\n");
    return ENOERR;
}

// ----------------------------------------------------------------------------

static Cyg_ErrNo
stm32_mmc_read_disk_block(cyg_mmc_stm32_disk_info_t * disk,
                cyg_uint8* buf, cyg_uint32 blocks, cyg_uint32 first_block)
{
    cyg_uint32 cmd;

    if (blocks == 0) {
        return  -EDOM;
    }


    /*!< Clear all DPSM configuration */
    stm32_sdio_data_config(SD_DATATIMEOUT,
                           0,/* data_len */
                           1,/* block_size */
                           1,/* to_card */
                           1,/* block_mode */
                           0);/* DPSM_ENABLE */
    if (stm32_sdio_get_response(1) & SD_CARD_LOCKED) {
        return -EIO;
    }

    /*!< Set Block Size for Card */
    stm32_sdio_send_command(MMC_STM32_BLOCK_SIZE,/* Argument */
                            SD_CMD_SET_BLOCKLEN,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    if (stm32_sdio_chk_cmd_resp1_error(SD_CMD_SET_BLOCKLEN) != SD_OK) {
        return -EIO;
    }

    stm32_sdio_data_config(SD_DATATIMEOUT,
                           MMC_STM32_BLOCK_SIZE * blocks,
                           MMC_STM32_BLOCK_SIZE,
                           0,
                           1,
                           1);

    /* Prepare to read data */
    stm32_dma_transaction_begin(disk,
                        true, false, buf, (blocks * MMC_STM32_BLOCK_SIZE));


    /* Startup SD/MMC card to send data to SDIO by send CMD17 or CMD18 */
    cmd = (blocks == 1 ? SD_CMD_READ_SINGLE_BLOCK : SD_CMD_READ_MULT_BLOCK);
    stm32_sdio_send_command((first_block * MMC_STM32_BLOCK_SIZE),/* Argument */
                            cmd,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    if (stm32_sdio_chk_cmd_resp1_error(cmd) != SD_OK) {
        return -EIO;
    }


    /* Waiting for the transfer to complete */
    stm32_dma_transaction(disk);
    stm32_dma_transaction_end(disk);


#if DEBUG > 2
    if (/*first_block == 477*/1) {
        int i;
        cyg_uint8 *ptr_data;

        diag_printf("\n*** Read SD/MMC card from block %d data dump::: \n", first_block);
        for (i = 0; i < 512; i += 16) {
            ptr_data = &disk->setup->bbuf[i];
            diag_printf(" %04x: %02x %02x %02x %02x  %02x %02x %02x %02x  "
                                "%02x %02x %02x %02x  %02x %02x %02x %02x\n",
                (i + (first_block * 512)),
                ptr_data[ 0], ptr_data[ 1], ptr_data[ 2], ptr_data[ 3],
                ptr_data[ 4], ptr_data[ 5], ptr_data[ 6], ptr_data[ 7],
                ptr_data[ 8], ptr_data[ 9], ptr_data[10], ptr_data[11],
                ptr_data[12], ptr_data[13], ptr_data[14], ptr_data[15]);
        }
        diag_printf("\n");
    }
#endif

    return ENOERR;
}

static Cyg_ErrNo
stm32_mmc_write_disk_block(cyg_mmc_stm32_disk_info_t * disk,
           const cyg_uint8 * buf, cyg_uint32 blocks, cyg_uint32 first_block)
{
    stm32_sd_error_t errorstatus = SD_OK;
    cyg_uint32 cmd, timeout;
    cyg_uint32 card_state;

    if (blocks == 0) {
        return  -EDOM;
    }


    /*!< Clear all DPSM configuration */
    stm32_sdio_data_config(SD_DATATIMEOUT,
                           0,
                           1,
                           1,
                           1,
                           0);
    if ((errorstatus = stm32_sdio_get_response(1)) & SD_CARD_LOCKED) {
        DEBUG1("Clear all DPSM configuration failed, errorstatus = %d\n", errorstatus);
        return -EIO;
    }

#if 0 // 经常出现CTIMEOUT，原因未知，所以把它屏蔽掉了
    /*!< Set Block Size for Card */
    stm32_sdio_send_command(MMC_STM32_BLOCK_SIZE,/* Argument */
                            SD_CMD_SET_BLOCKLEN,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_SET_BLOCKLEN);
    if (errorstatus != SD_OK) {
        DEBUG1("Set block size for card failed, errorstatus = %d\n", errorstatus);
        return -EIO;
    }
#endif

    /*!< Wait till card is ready for data Added */
    card_state = 0x00;
    timeout = SD_DATATIMEOUT;
    while (((card_state & 0x00000100) == 0) && (timeout > 0)) {
        timeout--;
        stm32_sdio_send_command(((cyg_uint32)(disk->rca << 16)),/* Argument */
                                SD_CMD_SEND_STATUS,/* Command */
                                CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                                CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                                true);
        errorstatus = stm32_sdio_chk_cmd_resp1_error(SD_CMD_SEND_STATUS);
        if (errorstatus != SD_OK) {
            DEBUG1("Wait till card is ready failed, errorstatus = %d\n", errorstatus);
            return -EIO;
        }

        card_state = stm32_sdio_get_response(1);
    }
    if (timeout == 0) {
        DEBUG1("Timeout for wait till card is ready\n");
        return -EIO;
    }


    /*!< Startup SD/MMC card to receive data to SDIO by send CMD24 or CMD25 */
    /*!< Send CMD24 WRITE_SINGLE_BLOCK */
    /*!< Send CMD25 WRITE_MULT_BLOCK with argument data address */
    cmd = (blocks == 1 ? SD_CMD_WRITE_SINGLE_BLOCK : SD_CMD_WRITE_MULT_BLOCK);
    stm32_sdio_send_command((first_block * MMC_STM32_BLOCK_SIZE),/* Argument */
                            cmd,/* Command */
                            CYGHWR_HAL_STM32_SDIO_CMD_SHORTRESP_CMDREND,
                            CYGHWR_HAL_STM32_SDIO_CMD_WAITNONE,
                            true);
    if ((errorstatus = stm32_sdio_chk_cmd_resp1_error(cmd)) != SD_OK) {
        DEBUG1("Startup SD/MMC card to receive data failed, "
               "errorstatus = %d\n",  errorstatus);
        return -EIO;
    }


    /* Prepare to send data */
    stm32_dma_transaction_begin(disk,
                        true, true, buf, (blocks * MMC_STM32_BLOCK_SIZE));


    stm32_sdio_data_config(SD_DATATIMEOUT,
                           MMC_STM32_BLOCK_SIZE * blocks,
                           MMC_STM32_BLOCK_SIZE,
                           1,
                           1,
                           1);


    /* Waiting for the transfer to complete */
    stm32_dma_transaction(disk);
    stm32_dma_transaction_end(disk);

#if DEBUG > 2
    if (/*first_block == 477*/1) {
        int i;
        cyg_uint8 *ptr_data;

        diag_printf("\n::: Write SD/MMC card to block %d data dump::: \n", first_block);
        for (i = 0; i < 512; i += 16) {
            ptr_data = &disk->setup->bbuf[i];
            diag_printf(" %04x: %02x %02x %02x %02x  %02x %02x %02x %02x  "
                                "%02x %02x %02x %02x  %02x %02x %02x %02x\n",
                (i + (first_block * 512)),
                ptr_data[ 0], ptr_data[ 1], ptr_data[ 2], ptr_data[ 3],
                ptr_data[ 4], ptr_data[ 5], ptr_data[ 6], ptr_data[ 7],
                ptr_data[ 8], ptr_data[ 9], ptr_data[10], ptr_data[11],
                ptr_data[12], ptr_data[13], ptr_data[14], ptr_data[15]);
        }
        diag_printf("\n");
    }
#endif


    return ENOERR;
}



// ----------------------------------------------------------------------------

// check_for_disk() tries to communicate with an MMC card that is not
// currently mounted. It performs the appropriate initialization so
// that read and write operations are possible, checks the disk format,
// distinguishes between read-only and read-write cards, calculates the
// card size, stores the unique id, etc.
//
// The main error conditions are ENODEV (no card), EIO (card not
// responding sensibly to requests), ENOTDIR (wrong format), or EPERM
// (card is password-locked).
static Cyg_ErrNo
stm32_mmc_check_for_disk(cyg_mmc_stm32_disk_info_t * disk)
{
    int                 i;
    Cyg_ErrNo           code;
    mmc_cid_register    *cid;
    mmc_csd_register    *csd;

#ifdef MMC_STM32_BACKGROUND_WRITES
    // If we have unmounted a disk and are remounting it, assume that
    // any writes have completed.
    disk->mmc_writing   = false;
#endif

    if (!stm32_mmc_card_detect(disk)) {
        return -ENODEV;
    }

    if (stm32_mmc_init(disk) != SD_OK) {
        return -EIO;
    }


    /* The card's unique ID */
    cid = (mmc_cid_register *)CID_Tab;
    DEBUG2("CID data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",               \
           cid->cid_data[ 0], cid->cid_data[ 1], cid->cid_data[ 2], cid->cid_data[ 3],  \
           cid->cid_data[ 4], cid->cid_data[ 5], cid->cid_data[ 6], cid->cid_data[ 7],  \
           cid->cid_data[ 8], cid->cid_data[ 9], cid->cid_data[10], cid->cid_data[11],  \
           cid->cid_data[12], cid->cid_data[13], cid->cid_data[14], cid->cid_data[15]);
#if DEBUG > 0
    DEBUG1("CID data: register\n");
    DEBUG1("        : Manufacturer ID       : MID = 0x%02x\n", MMC_CID_REGISTER_MID(cid) & 0xff);
    DEBUG1("        : OEM/Application ID    : OID = 0x%04x\n", MMC_CID_REGISTER_OID(cid) & 0xffff);
    DEBUG1("        : Product name          : PNM = 0x%02x%02x%02x%02x%02x%02x\n",
                                                               MMC_CID_REGISTER_PNM(cid)[0] & 0xff,
                                                               MMC_CID_REGISTER_PNM(cid)[1] & 0xff,
                                                               MMC_CID_REGISTER_PNM(cid)[2] & 0xff,
                                                               MMC_CID_REGISTER_PNM(cid)[3] & 0xff,
                                                               MMC_CID_REGISTER_PNM(cid)[4] & 0xff,
                                                               MMC_CID_REGISTER_PNM(cid)[5] & 0xff);
    DEBUG1("        : Product revision      : PRV = 0x%02x\n", MMC_CID_REGISTER_PRV(cid) & 0xff);
    DEBUG1("        : Product serial number : PSN = 0x%08x\n", MMC_CID_REGISTER_PSN(cid) & 0xffffffff);
    DEBUG1("        : Manufacturing date    : MDT = 0x%02x\n", MMC_CID_REGISTER_MDT(cid) & 0xff);
    DEBUG1("        : 7-bit CRC checksum    : CRC = 0x%02x\n", MMC_CID_REGISTER_CRC(cid) & 0xff);
#endif

    /* And retrieve the card's configuration data */
    csd = (mmc_csd_register *)CSD_Tab;
    DEBUG2("CSD data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",   \
           csd->csd_data[ 0], csd->csd_data[ 1], csd->csd_data[ 2], csd->csd_data[3],   \
           csd->csd_data[ 4], csd->csd_data[ 5], csd->csd_data[ 6], csd->csd_data[7],   \
           csd->csd_data[ 8], csd->csd_data[ 9], csd->csd_data[10], csd->csd_data[11],  \
           csd->csd_data[12], csd->csd_data[13], csd->csd_data[14], csd->csd_data[15]);

    // Optionally dump the whole CSD register. This takes a lot of
    // code but gives a lot of info about the card. If the info looks
    // correct then we really are interacting properly with an MMC card.
#if DEBUG > 0
    DEBUG1("CSD data: structure 0x%02x, version 0x%02x\n", MMC_CSD_REGISTER_CSD_STRUCTURE(csd), MMC_CSD_REGISTER_SPEC_VERS(csd));
    if (0 != MMC_CSD_REGISTER_FILE_FORMAT_GROUP(csd)) {
        DEBUG1("        : Reserved (unknown), FILE_FORMAT_GROUP %d, FILE_FORMAT %d\n", \
                    MMC_CSD_REGISTER_FILE_FORMAT_GROUP(csd), MMC_CSD_REGISTER_FILE_FORMAT(csd));
    } else if (0 == MMC_CSD_REGISTER_FILE_FORMAT(csd)) {
        DEBUG1("        : Partioned disk, FILE_FORMAT_GROUP 0, FILE_FORMAT 0\n");
    } else if (1 == MMC_CSD_REGISTER_FILE_FORMAT(csd)) {
        DEBUG1("        : FAT disk, FILE_FORMAT_GROUP 0, FILE_FORMAT 1\n");
    } else if (2 == MMC_CSD_REGISTER_FILE_FORMAT(csd)) {
        DEBUG1("        : Universal File format, FILE_FORMAT_GROUP 0, FILE_FORMAT 2\n");
    } else {
        DEBUG1("        : Others/Unknown disk, FILE_FORMAT_GROUP 0, FILE_FORMAT 3\n");
    }
    {
        static const cyg_uint32 mantissa_speeds_x10[16]   = { 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
        static const cyg_uint32 exponent_speeds_div10[8]  = { 10000, 100000, 1000000, 10000000, 0, 0, 0, 0 };
        cyg_uint32 speed = mantissa_speeds_x10[MMC_CSD_REGISTER_TRAN_SPEED_MANTISSA(csd)] *
            exponent_speeds_div10[MMC_CSD_REGISTER_TRAN_SPEED_EXPONENT(csd)];
        speed /= 1000;
        DEBUG1("        : TRAN_SPEED %d %d -> %d kbit/s\n", \
               MMC_CSD_REGISTER_TRAN_SPEED_MANTISSA(csd), MMC_CSD_REGISTER_TRAN_SPEED_EXPONENT(csd), speed);
    }
    DEBUG1("        : READ_BL_LEN block length 2^%d (%d)\n", MMC_CSD_REGISTER_READ_BL_LEN(csd), \
                0x01 << MMC_CSD_REGISTER_READ_BL_LEN(csd));
    DEBUG1("        : C_SIZE %d, C_SIZE_MULT %d\n", MMC_CSD_REGISTER_C_SIZE(csd), MMC_CSD_REGISTER_C_SIZE_MULT(csd));
    {
        cyg_uint32 block_len = 0x01 << MMC_CSD_REGISTER_READ_BL_LEN(csd);
        cyg_uint32 mult      = 0x01 << (MMC_CSD_REGISTER_C_SIZE_MULT(csd) + 2);
        cyg_uint32 size      = block_len * mult * (MMC_CSD_REGISTER_C_SIZE(csd) + 1);
        cyg_uint32 sizeK     = (cyg_uint32) (size / 1024);
        cyg_uint32 sizeM     =  sizeK / 1024;
        sizeK  -= (sizeM * 1024);
        DEBUG1("        : total card size %dM%dK\n", sizeM, sizeK);
    }
    DEBUG1("        : WR_BL_LEN block length 2^%d (%d)\n", \
           MMC_CSD_REGISTER_WRITE_BL_LEN(csd), 0x01 << MMC_CSD_REGISTER_WRITE_BL_LEN(csd));
    {
        static cyg_uint32 taac_mantissa_speeds_x10[16]   = { 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80 };
        static cyg_uint32 taac_exponent_speeds_div10[8]  = { 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };
        cyg_uint32 taac_speed = taac_mantissa_speeds_x10[MMC_CSD_REGISTER_TAAC_MANTISSA(csd)] *
            taac_exponent_speeds_div10[MMC_CSD_REGISTER_TAAC_EXPONENT(csd)];
        taac_speed /= 100;
        DEBUG1("        : asynchronous read access time TAAC %d %d -> %d ns\n", \
               MMC_CSD_REGISTER_TAAC_MANTISSA(csd), MMC_CSD_REGISTER_TAAC_EXPONENT(csd), taac_speed);
    }
    DEBUG1("        : synchronous read access time NSAC %d * 100 cycles\n", \
           MMC_CSD_REGISTER_NSAC(csd));
    DEBUG1("        : typical write program time %d * read time\n", MMC_CSD_REGISTER_R2W_FACTOR(csd));
    DEBUG1("        : CCC command classes 0x%04x\n", MMC_CSD_REGISTER_CCC(csd));
    DEBUG1("        : READ_BL_PARTIAL %d, WRITE_BLK_MISALIGN %d, READ_BLK_MISALIGN %d, DSR_IMP %d\n",   \
           MMC_CSD_REGISTER_READ_BL_PARTIAL(csd), MMC_CSD_REGISTER_WRITE_BLK_MISALIGN(csd),           \
           MMC_CSD_REGISTER_READ_BLK_MISALIGN(csd), MMC_CSD_REGISTER_DSR_IMP(csd));
    DEBUG1("        : WR_BL_PARTIAL %d\n", MMC_CSD_REGISTER_WR_BL_PARTIAL(csd));
    {
        static cyg_uint8    min_currents[8] = { 1, 1, 5, 10, 25, 35, 60, 100 };
        static cyg_uint8    max_currents[8] = { 1, 5, 10, 25, 35, 45, 80, 200 };
        DEBUG1("        : read current min %dmA, max %dmA\n",               \
                    min_currents[MMC_CSD_REGISTER_VDD_R_CURR_MIN(csd)],    \
                    max_currents[MMC_CSD_REGISTER_VDD_R_CURR_MAX(csd)]);
        DEBUG1("        : write current min %dmA, max %dmA\n",              \
                    min_currents[MMC_CSD_REGISTER_VDD_W_CURR_MIN(csd)],    \
                    max_currents[MMC_CSD_REGISTER_VDD_W_CURR_MAX(csd)]);
    }
    DEBUG1("        : erase sector size %d, erase group size %d\n", \
           MMC_CSD_REGISTER_SECTOR_SIZE(csd) + 1, MMC_CSD_REGISTER_ERASE_GRP_SIZE(csd) + 1);
    DEBUG1("        : write group enable %d, write group size %d\n", \
           MMC_CSD_REGISTER_WR_GRP_ENABLE(csd), MMC_CSD_REGISTER_WR_GRP_SIZE(csd) + 1);
    DEBUG1("        : copy bit %d\n", MMC_CSD_REGISTER_COPY(csd));
    DEBUG1("        : permanent write protect %d, temporary write protect %d\n", \
           MMC_CSD_REGISTER_PERM_WRITE_PROTECT(csd), MMC_CSD_REGISTER_TMP_WRITE_PROTECT(csd));
    DEBUG1("        : ecc %d, default ecc %d\n", MMC_CSD_REGISTER_ECC(csd), MMC_CSD_REGISTER_DEFAULT_ECC(csd));
    DEBUG1("        : crc 0x%08x\n", MMC_CSD_REGISTER_CRC(csd));
#endif

    // There is information available about the file format, e.g.
    // partitioned vs. simple FAT. With the current version of the
    // generic disk code this needs to be known statically, via
    // the mbr field of the disk channel structure. If the card
    // is inappropriately formatted, reject the mount request.
    if ((0 != MMC_CSD_REGISTER_FILE_FORMAT_GROUP(csd)) ||
        (0 != MMC_CSD_REGISTER_FILE_FORMAT(csd))) {
        DEBUG1("Error, NOTDIR!\n");
//      return -ENOTDIR;
    }

    // Look for a write-protect bit (permanent or temporary), and set
    // the disk as read-only or read-write as appropriate. The
    // temporary write-protect could be cleared by rewriting the CSD
    // register (including recalculating the CRC) but the effort
    // involves does not seem worth-while.
    if ((0 != MMC_CSD_REGISTER_PERM_WRITE_PROTECT(csd)) ||
        (0 != MMC_CSD_REGISTER_TMP_WRITE_PROTECT(csd))) {
        disk->mmc_read_only   = true;
    } else {
        disk->mmc_read_only   = false;
    }
    DEBUG1("Disk read-only flag %d\n", disk->mmc_read_only);


    // Calculate the disk size, primarily for assertion purposes.
    // By design MMC cards are limited to 4GB, which still doesn't
    // quite fit into 32 bits.
#if 0
    disk->mmc_block_count = (((cyg_uint64)(0x01 << MMC_CSD_REGISTER_READ_BL_LEN(csd))) *
                             ((cyg_uint64)(0x01 << (MMC_CSD_REGISTER_C_SIZE_MULT(csd) + 2))) *
                             ((cyg_uint64)(MMC_CSD_REGISTER_C_SIZE(csd) + 1))) / (cyg_uint64)MMC_STM32_BLOCK_SIZE;
#else
    cyg_uint32 device_size_mul = (disk->csd.DeviceSizeMul + 2);
    if (disk->card_type == SD_CARD_TYPE_HIGH_CAPACITY_SD_CARD) {
        /* 高容量SD卡 SDHC */
        disk->mmc_block_count = (disk->csd.DeviceSize + 1) * 1024;
    } else {
        /* 普通SD卡, 最大4G */
        cyg_uint32 number_of_blocks = ((1 << disk->csd.RdBlockLen) / MMC_STM32_BLOCK_SIZE);
		disk->mmc_block_count = ((disk->csd.DeviceSize + 1) * (1 << device_size_mul)
                                    << (number_of_blocks / 2));
        DEBUG1("number_of_blocks = %d, device_size_mul= %d\n",
                number_of_blocks, device_size_mul);
    }

#endif
    DEBUG1("Disk block counts: %d (0x%08x)\n", disk->mmc_block_count, disk->mmc_block_count);

    // Assume for now that the block length is 512 bytes. This is
    // probably a safe assumption since we have just got the card
    // initialized out of idle state. If it ever proves to be a problem
    // the SET_BLOCK_LEN command can be used.
    // Nevertheless store the underlying block sizes
#if 0
    disk->mmc_read_block_length  = 0x01 << MMC_CSD_REGISTER_READ_BL_LEN(csd);
    disk->mmc_write_block_length = 0x01 << MMC_CSD_REGISTER_WRITE_BL_LEN(csd);
#else
    disk->mmc_read_block_length = MMC_STM32_BLOCK_SIZE;
    disk->mmc_write_block_length= MMC_STM32_BLOCK_SIZE;
#endif
    DEBUG1("Disk mmc_read_block_length = %d\n", disk->mmc_read_block_length);
    DEBUG1("Disk mmc_write_block_length= %d\n", disk->mmc_write_block_length);


    // Read the partition table off the card. This is a way of
    // checking that the card is not password-locked. It also
    // provides information about the "disk geometry" which is
    // needed by higher-level code.
    // FIXME: the higher-level code should be made to use LBA
    // addressing instead.
    {
        cyg_uint8   data[MMC_STM32_BLOCK_SIZE];
        cyg_uint8*  partition;
        cyg_uint32  lba_first, lba_size, lba_end, head, cylinder, sector;

        code = stm32_mmc_read_disk_block(disk, data, 1, 0);
        if (ENOERR != code) {
            DEBUG1("Error, read disk block0 failed!\n");
            return code;
        }
#if DEBUG > 1
        {
            cyg_uint8 *ptr_data;

            DEBUG2("MBR dump\n");
            for (i = 0; i < MMC_STM32_BLOCK_SIZE; i += 16) {
                ptr_data = &data[i];
                DEBUG2(" %04x: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
                    i,
                    ptr_data[ 0], ptr_data[ 1], ptr_data[ 2], ptr_data[ 3],
                    ptr_data[ 4], ptr_data[ 5], ptr_data[ 6], ptr_data[ 7],
                    ptr_data[ 8], ptr_data[ 9], ptr_data[10], ptr_data[11],
                    ptr_data[12], ptr_data[13], ptr_data[14], ptr_data[15]);
            }
        }
#endif
#if DEBUG > 0
        DEBUG1("Read block 0 (partition table)\n");
        DEBUG1("Signature 0x%02x 0x%02x, should be 0x55 0xaa\n", data[0x1fe], data[0x1ff]);
        // There should be four 16-byte partition table entries at offsets
        // 0x1be, 0x1ce, 0x1de and 0x1ee. The numbers are stored little-endian
        for (i = 0; i < 4; i++) {
            partition = &(data[0x1be + (0x10 * i)]);
            DEBUG1("Partition %d: boot %02x, first CHS %02x, last CHS %02x, "
                   "first sector %02x %02x %02x, file system %02x, "
                   "last sector %02x %02x %02x\n",
                   i,   \
                   partition[0], \
                   ((partition[2] & 0xC0) << 2) | partition[3],\
                   ((partition[6] & 0xC0) << 2) | partition[7], \
                   partition[1], partition[2], partition[3], partition[4], \
                   partition[5], partition[6], partition[7]);

            DEBUG1("           : first sector (linear) %02x %02x %02x %02x, "
                                "sector count %02x %02x %02x %02x\n", \
                   partition[11], partition[10], partition[9], partition[8], \
                   partition[15], partition[14], partition[13], partition[12]);
        }
#endif
        if ((0x0055 != data[0x1fe]) || (0x00aa != data[0x1ff])) {
            return -ENOTDIR;
        }
        partition   = &(data[0x1be]);
        lba_first   = (partition[11] << 24) | (partition[10] << 16) | (partition[9] << 8) | partition[8];
        lba_size    = (partition[15] << 24) | (partition[14] << 16) | (partition[13] << 8) | partition[12];
        lba_end     = lba_first + lba_size - 1;

        // First sector in c/h/s format
        cylinder    = ((partition[2] & 0xC0) << 2) | partition[3];
        head        = partition[1];
        sector      = partition[2] & 0x3F;

        // lba_start == (((cylinder * Nh) + head) * Ns) + sector - 1, where (Nh == heads/cylinder) and (Ns == sectors/head)
        // Strictly speaking we should be solving some simultaneous
        // equations here for lba_start/lba_end, but that gets messy.
        // The first partition is at the start of the card so cylinder will be 0,
        // and we can ignore Nh.
        CYG_ASSERT(0 == cylinder, "Driver assumption - partition 0 is at start of card\n");
        CYG_ASSERT(0 != head,     "Driver assumption - partition table is sensible\n");
        disk->mmc_sectors_per_head = ((lba_first + 1) - sector) / head;

        // Now for lba_end.
        cylinder    = ((partition[6] & 0xC0) << 2) | partition[7];
        head        = partition[5];
        sector      = partition[6] & 0x3F;
        disk->mmc_heads_per_cylinder = ((((lba_end + 1) - sector) / disk->mmc_sectors_per_head) - head) / cylinder;
    }

    return ENOERR;
}

// Check that the current card is the one that was previously
// accessed. This may fail if the card has been removed and the
// slot is empty, or if the card has been removed and a different
// one inserted. It may pass incorrectly if a card is removed,
// modified elsewhere, and reinserted without eCos noticing.
// There is no way around that without some way of detecting
// disk removal in hardware.
//
// Re-reading the cid may actually be overkill. If a new card
// has been plugged in then it will not have been initialized so
// it will respond with 0xff anyway. It is very unlikely that
// an init sequence will have happened by accident.
static cyg_bool
stm32_mmc_disk_changed(cyg_mmc_stm32_disk_info_t* disk)
{
    cyg_uint32*     old_cid_tab;
    cyg_uint32      new_cid_tab[4];

    if (!stm32_mmc_card_detect(disk)) {
        return  true;// SD/MMC card is removed and the slot is empty.
    }


    /* Is a new card? */

    old_cid_tab = CID_Tab;

    // Read the card CID.
    if (stm32_mmc_get_card_cid(SD_CMD_SEND_CID, new_cid_tab) != SD_OK) {
        DEBUG1("Read the card CID failed.\n");
        return true;
    }

    // Compare the card CID.
    if (0 != memcmp(new_cid_tab, old_cid_tab, sizeof(new_cid_tab))) {
        DEBUG1("A new SD/MMC card.\n");
        DEBUG1("Old MMC/SD card CID = %08X_%08X_%08X_%08X\n",
                old_cid_tab[3], old_cid_tab[2], old_cid_tab[1], old_cid_tab[0]);
        DEBUG1("New MMC/SD card CID = %08X_%08X_%08X_%08X\n",
                new_cid_tab[3], new_cid_tab[2], new_cid_tab[1], new_cid_tab[0]);
        return true;/* A new SD/MMC card */
    }
    return false;
}


// ----------------------------------------------------------------------------
// --------------------------- DISK Driver ------------------------------------
// ----------------------------------------------------------------------------


// No hardware initialization is performed here. Even if a card is
// currently plugged in it may get removed before it gets mounted, so
// there is no point looking at the card here. It is still necessary
// to invoke the callback init function so that higher-level code gets
// a chance to do its bit.
static cyg_bool
mmc_stm32_disk_init(struct cyg_devtab_entry* tab)
{
    disk_channel * chan = (disk_channel*) tab->priv;
    cyg_mmc_stm32_disk_info_t* disk = (cyg_mmc_stm32_disk_info_t*)chan->dev_priv;

    DEBUG1("Disk init.\n");

    /* Low level init */

    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_SDIO_PIN_DAT0);
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_SDIO_PIN_DAT1);
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_SDIO_PIN_DAT2);
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_SDIO_PIN_DAT3);
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_SDIO_PIN_CLK);
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_SDIO_PIN_CMD);
    CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_SDIO_PIN_DAT0);

    /*!< Configure SD Card detect pin */
    CYGHWR_HAL_STM32_GPIO_SET( STM32_MMC_CARD_DETECT_PIN );


    /*!< Enable the SDIO AHB Clock */
    CYGHWR_HAL_STM32_CLOCK_ENABLE( CYGHWR_HAL_STM32_SDIO_CLOCK );


    // Initialise the synchronisation primitivies.
    cyg_drv_mutex_init (&disk->mutex);
    cyg_drv_cond_init (&disk->condvar, &disk->mutex);

    cyg_mutex_init(&disk->wr_mutex);

    // Initialize DMA streams
    hal_stm32_dma_init(&disk->dma_tx_stream, disk->setup->dma_tx_intr_pri);
    hal_stm32_dma_init(&disk->dma_rx_stream, disk->setup->dma_rx_intr_pri);

    return (*chan->callbacks->disk_init)(tab);
}

// lookup() is called during a mount() operation, so this is the right
// place to check whether or not there is a card.

static char*
mmc_stm32_disk_lookup_itoa(cyg_uint32 num, char* where)
{
    if (0 == num) {
        *where++ = '0';
    } else {
        char local[10];  // 2^32 just fits into 10 places
        int  index = 9;
        while (num > 0) {
            local[index--] = (num % 10) + '0';
            num /= 10;
        }
        for (index += 1; index < 10; index++) {
            *where++ = local[index];
        }
    }
    return where;
}

static Cyg_ErrNo
mmc_stm32_disk_lookup(struct cyg_devtab_entry** tab,
                struct cyg_devtab_entry *sub_tab, const char* name)
{
    disk_channel*              chan = (disk_channel*) (*tab)->priv;
    cyg_mmc_stm32_disk_info_t* disk = (cyg_mmc_stm32_disk_info_t*)chan->dev_priv;
    Cyg_ErrNo                  result;

    DEBUG2("mmc_stm32_disk_lookup(): target name=%s\n", name ? name : "NULL");
    DEBUG2("                       : device name=%s dep_name=%s\n",
                  tab[0]->name, (tab[0]->dep_name ? tab[0]->dep_name : "NULL"));

    if (disk->mmc_connected) {
        // There was a card plugged in last time we looked. Is it still there?
        if (stm32_mmc_disk_changed(disk)) {
            // The old card is gone. Either there is no card plugged in, or
            // it has been replaced with a different one. If the latter the
            // existing mounts must be removed before anything sensible
            // can be done.
            disk->mmc_connected = false;
            (*chan->callbacks->disk_disconnected)(chan);
            if (0 != chan->info->mounts) {
                return -ENODEV;
            }
        }
    }

    if ((0 != chan->info->mounts) && !disk->mmc_connected) {
        // There are still mount points to an old card. We cannot accept
        // new mount requests until those have been cleaned out.
        return -ENODEV;
    }


    if (!disk->mmc_connected) {
        cyg_disk_identify_t ident;
        cyg_uint32          id_data;
        char*               where;
        int                 i;
        mmc_cid_register*   cid = (mmc_cid_register *)CID_Tab;

        // The world is consistent and the higher-level code does not
        // know anything about the current card, if any. Is there a
        // card?
        result = stm32_mmc_check_for_disk(disk);
        if (ENOERR != result) {
            return result;
        }

        // A card has been found. Tell the higher-level code about it.
        // This requires an identify structure, although it is not
        // entirely clear what purpose that serves.
        disk->mmc_connected = true;
        // Serial number, up to 20 characters; The CID register contains
        // various fields which can be used for this.
        where   = &(ident.serial[0]);
        id_data = cid->cid_data[0];   // 1-byte manufacturer id -> 3 chars, 17 left
        where   = mmc_stm32_disk_lookup_itoa(id_data, where);
        id_data = (cid->cid_data[1] << 8) + cid->cid_data[2]; // 2-byte OEM ID, 5 chars, 12 left
        where   = mmc_stm32_disk_lookup_itoa(id_data, where);
        id_data = (cid->cid_data[10] << 24) + (cid->cid_data[11] << 16) +
            (cid->cid_data[12] << 8) + cid->cid_data[13];
        where   = mmc_stm32_disk_lookup_itoa(id_data, where); // 4-byte OEM ID, 10 chars, 2 left
        // And terminate the string with a couple of places to spare.
        *where = '\0';

        // Firmware revision number. There is a one-byte product
        // revision number in the CID, BCD-encoded
        id_data = cid->cid_data[9] >> 4;
        if (id_data <= 9) {
            ident.firmware_rev[0] = id_data + '0';
        } else {
            ident.firmware_rev[0] = id_data - 10 + 'A';
        }
        id_data = cid->cid_data[9] & 0x0F;
        if (id_data <= 9) {
            ident.firmware_rev[1] = id_data + '0';
        } else {
            ident.firmware_rev[1] = id_data - 10 + 'A';
        }
        ident.firmware_rev[2]   = '\0';

        // Model number. There is a six-byte product name in the CID.
        for (i = 0; i < 6; i++) {
            if ((cid->cid_data[i + 3] >= 0x20) && (cid->cid_data[i+3] <= 0x7E)) {
                ident.model_num[i] = cid->cid_data[i + 3];
            } else {
                break;
            }
        }
        ident.model_num[i] = '\0';

        // We don't have no cylinders, heads, or sectors, but
        // higher-level code may interpret partition data using C/H/S
        // addressing rather than LBA. Hence values for some of these
        // settings were calculated above.
        ident.cylinders_num     = 1;
        ident.heads_num         = disk->mmc_heads_per_cylinder;
        ident.sectors_num       = disk->mmc_sectors_per_head;
        ident.lba_sectors_num   = disk->mmc_block_count;
        ident.phys_block_size   = disk->mmc_write_block_length/512;
        ident.max_transfer      = disk->mmc_write_block_length;

        DEBUG1("-----------------------------------------------------------\n");
        DEBUG1("Disk ident: \n");
        DEBUG1("\t cylinders_num  : %u\n", ident.cylinders_num);
        DEBUG1("\t heads_num      : %u\n", ident.heads_num);
        DEBUG1("\t sectors_num    : %u\n", ident.sectors_num);
        DEBUG1("\t lba_sectors_num: %u\n", ident.lba_sectors_num);
        DEBUG1("\t phys_block_size: %u\n", ident.phys_block_size);
        DEBUG1("\t max_transfer   : %u\n", ident.max_transfer);
        DEBUG1("-- ---------------------------------------------------------\n");

        DEBUG1("Calling disk_connected(): serial %s, firmware %s, model %s, "
                "heads %d, sectors %d, lba_sectors_num %d, phys_block_size %d\n", \
               ident.serial, ident.firmware_rev, ident.model_num, ident.heads_num, ident.sectors_num,
               ident.lba_sectors_num, ident.phys_block_size);
        (*chan->callbacks->disk_connected)(*tab, &ident);

        // We now have a valid card and higher-level code knows about it. Fall through.
    }


    // And leave it to higher-level code to finish the lookup, taking
    // into accounts partitions etc.
    return (*chan->callbacks->disk_lookup)(tab, sub_tab, name);
}

static Cyg_ErrNo
mmc_stm32_disk_read(disk_channel* chan,
                void* buf_arg, cyg_uint32 blocks, cyg_uint32 first_block)
{
    cyg_mmc_stm32_disk_info_t* disk = (cyg_mmc_stm32_disk_info_t*)chan->dev_priv;
    cyg_uint32                 i;
    cyg_uint8*                 buf  = (cyg_uint8*) buf_arg;
    Cyg_ErrNo                  code = ENOERR;

    DEBUG1("mmc_stm32_disk_read(): first block %d, buf %p, len %lu blocks (%lu bytes)\n",
           first_block, buf, (unsigned long)blocks, (unsigned long)blocks*512);

    if (! disk->mmc_connected) {
        return -ENODEV;
    }
    if ((first_block + blocks) >= disk->mmc_block_count) {
        return -EINVAL;
    }

    for (i = 0; (i < blocks) && (ENOERR == code); i++) {
        code = stm32_mmc_read_disk_block(disk, buf, 1, first_block + i);
        buf += MMC_STM32_BLOCK_SIZE;
    }
    return  code;
}

static Cyg_ErrNo
mmc_stm32_disk_write(disk_channel* chan,
                const void* buf_arg, cyg_uint32 blocks, cyg_uint32 first_block)
{
    cyg_mmc_stm32_disk_info_t* disk = (cyg_mmc_stm32_disk_info_t*)chan->dev_priv;
    cyg_uint32                 i;
    const cyg_uint8*           buf  = (cyg_uint8*) buf_arg;
    Cyg_ErrNo                  code = ENOERR;

    DEBUG1("mmc_stm32_disk_write(): first block %d, buf %p, len %lu blocks (%lu bytes)\n",
           first_block, buf, (unsigned long)blocks, (unsigned long)blocks*512);

    if (! disk->mmc_connected) {
        return -ENODEV;
    }
    if (disk->mmc_read_only) {
        return -EROFS;
    }
    if ((first_block + blocks) >= disk->mmc_block_count) {
        return -EINVAL;
    }

    for (i = 0; (i < blocks) && (ENOERR == code); i++) {
        code = stm32_mmc_write_disk_block(disk, buf, 1, first_block + i);
        buf += MMC_STM32_BLOCK_SIZE;
    }
    DEBUG1("code = %d\n", code);
    return code;
}

// get_config() and set_config(). There are no supported get_config() operations
// at this time.
static Cyg_ErrNo
mmc_stm32_disk_get_config(disk_channel* chan,
                            cyg_uint32 key, const void* buf, cyg_uint32* len)
{
    CYG_UNUSED_PARAM(disk_channel*, chan);
    CYG_UNUSED_PARAM(cyg_uint32, key);
    CYG_UNUSED_PARAM(const void*, buf);
    CYG_UNUSED_PARAM(cyg_uint32*, len);

    return -EINVAL;
}

static Cyg_ErrNo
mmc_stm32_disk_set_config(disk_channel* chan,
                            cyg_uint32 key, const void* buf, cyg_uint32* len)
{
    Cyg_ErrNo                  result = ENOERR;
    cyg_mmc_stm32_disk_info_t* disk = (cyg_mmc_stm32_disk_info_t*)chan->dev_priv;

    switch(key) {
      case CYG_IO_SET_CONFIG_DISK_MOUNT:
        // There will have been a successful lookup(), so there's
        // little point in checking the disk again.
        break;

      case CYG_IO_SET_CONFIG_DISK_UMOUNT:
        if (0 == chan->info->mounts) {
            // If this is the last unmount of the card, mark it as
            // disconnected. If the user then removes the card and
            // plugs in a new one everything works cleanly.
            disk->mmc_connected = false;
            result = (chan->callbacks->disk_disconnected)(chan);
        }
        break;
    }

    return result;
}


// ----------------------------------------------------------------------------
// And finally the data structures that define this disk. Some of this
// should be moved into an exported header file so that applications can
// define additional disks.
//
// It is not obvious why there are quite so many structures. Apart
// from the devtab entries there are no tables involved, so there is
// no need to keep everything the same size. The cyg_disk_info_t could
// be the common part of a h/w info_t. The channel structure is
// redundant and its fields could be merged into the cyg_disk_info_t
// structure. That would leave a devtab entry, a disk info structure
// (h/w specific but with a common base), and a disk controller
// structure (ditto).

DISK_FUNS(cyg_mmc_stm32_disk_funs,
          mmc_stm32_disk_read,
          mmc_stm32_disk_write,
          mmc_stm32_disk_get_config,
          mmc_stm32_disk_set_config
          );

static cyg_mmc_stm32_disk_info_t cyg_mmc_stm32_disk0_hwinfo = {
    .setup                  = &stm32_sdio_setup,

    .card_type              = SD_CARD_TYPE_STD_CAPACITY_SD_CARD_V1_1,
#ifdef MMC_STM32_BACKGROUND_WRITES
    .mmc_writing            = 0,
#endif
    .mmc_connected          = 0,

    .dma_tx_stream.desc     = CYGHWR_HAL_STM32_SDIO_DMA_TX,
    .dma_tx_stream.callback = stm32_dma_callback,
    .dma_tx_stream.data     = (CYG_ADDRWORD)&cyg_mmc_stm32_disk0_hwinfo,

    .dma_rx_stream.desc     = CYGHWR_HAL_STM32_SDIO_DMA_RX,
    .dma_rx_stream.callback = stm32_dma_callback,
    .dma_rx_stream.data     = (CYG_ADDRWORD)&cyg_mmc_stm32_disk0_hwinfo,

};

// No h/w controller structure is needed, but the address of the
// second argument is taken anyway.
DISK_CONTROLLER(cyg_mmc_stm32_disk_controller_0, cyg_mmc_stm32_disk0_hwinfo);

DISK_CHANNEL(cyg_mmc_stm32_disk0_channel,
             cyg_mmc_stm32_disk_funs,
             cyg_mmc_stm32_disk0_hwinfo,
             cyg_mmc_stm32_disk_controller_0,
             true,                            /* MBR support */
             1                                /* Number of partitions supported */
             );

BLOCK_DEVTAB_ENTRY(cyg_mmc_stm32_disk0_devtab_entry,
                   CYGDAT_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_NAME,
                   0,
                   &cyg_io_disk_devio,
                   &mmc_stm32_disk_init,
                   &mmc_stm32_disk_lookup,
                   &cyg_mmc_stm32_disk0_channel);


// ----------------------------------------------------------------------------
// EOF mmc_stm32.c
