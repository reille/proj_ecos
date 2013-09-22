//==========================================================================
//
//      i2s_stm32.c
//
//      I2S driver for STM32
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2010 Free Software Foundation, Inc.
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
// Author(s):    reille
// Contributors:
// Date:         2013-06-02
// Description:  I2S bus driver for STM32
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/devs_sound_i2s_cortexm_stm32.h>
#include <pkgconf/io.h>

#include <cyg/hal/hal_io.h>/* Pin configuration and GPIO definitions */
#include <cyg/hal/hal_if.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/drv_api.h>

#include <cyg/kernel/kapi.h>
#include <cyg/infra/cyg_ass.h>      // assertion support
#include <cyg/infra/diag.h>
#include <cyg/io/devtab.h>
#include <cyg/io/io.h>
#include <cyg/io/config_keys.h>

#include <cyg/io/i2s_stm32.h>

#include <string.h>/* for memcpy, memset and so on */


//-----------------------------------------------------------------------------
// Diagnostics

#if CYGDAT_DEVS_SOUND_I2S_CORTEXM_STM32_TRACE
#define i2s_diag( __fmt, ... ) \
 diag_printf("I2S: %s[%3d]: " __fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__ );
#define i2s_dump_buf( __addr, __size ) diag_dump_buf( __addr, __size )
#else
#define i2s_diag( __fmt, ... )
#define i2s_dump_buf( __addr, __size )
#endif


//-----------------------------------------------------------------------------
// Bus frequencies

__externC cyg_uint32 hal_stm32_sysclk;


//-----------------------------------------------------------------------------
// API function call forward references.

static bool      i2s_stm32_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo i2s_stm32_lookup(struct cyg_devtab_entry **tab,
                           struct cyg_devtab_entry *st,
                           const char *name);
static Cyg_ErrNo i2s_stm32_write(cyg_io_handle_t handle,
                           const void *buf,
                           cyg_uint32 *len);
static Cyg_ErrNo i2s_stm32_read(cyg_io_handle_t handle,
                           void *buffer,
                           cyg_uint32 *len);
static Cyg_ErrNo i2s_stm32_set_config(cyg_io_handle_t handle,
                           cyg_uint32 key,
                           const void *buffer,
                           cyg_uint32 *len);
static Cyg_ErrNo i2s_stm32_get_config(cyg_io_handle_t handle,
                           cyg_uint32 key,
                           void *buffer,
                           cyg_uint32 *len);

//-----------------------------------------------------------------------------
// Instantiate Device

#include CYGDAT_DEVS_SOUND_I2S_CORTEXM_STM32_INL


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

#if (defined (CYGHWR_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2) && \
             (CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE > 0)) || \
    (defined (CYGHWR_DEVS_SOUND_I2S_CORTEXM_STM32_BUS3) && \
             (CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS3_BBUF_SIZE > 0))
static cyg_uint16 dma_null __attribute__((section (".sram"))) = 0xFFFF;
#else
static cyg_uint16 dma_null = 0xFFFF;
#endif


//-----------------------------------------------------------------------------

// Instantiate the I2S bus state data structures.

#ifdef CYGHWR_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2

static void stm32_i2s_bus2_pin_setup(void * _stm32_bus_)
{
    cyg_i2s_cortexm_stm32_bus_t* stm32_bus = (cyg_i2s_cortexm_stm32_bus_t*)_stm32_bus_;

    if (stm32_bus->conf->i2s_mode == I2S_MODE_MASTER_SENDER ||
        stm32_bus->conf->i2s_mode == I2S_MODE_MASTER_RECEIVER) {
        if (stm32_bus->setup->mck_enable) {
            CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_I2S2_MCK_OUT);
        }
        CYGHWR_HAL_STM32_GPIO_SET((stm32_bus->conf->i2s_mode == I2S_MODE_MASTER_SENDER ?
                                    CYGHWR_HAL_STM32_I2S2_SD_OUT :
                                    CYGHWR_HAL_STM32_I2S2_SD_IN));
        CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_I2S2_WS_OUT);
        CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_I2S2_CK_OUT);

    } else if (stm32_bus->conf->i2s_mode == I2S_MODE_SLAVE_SENDER ||
               stm32_bus->conf->i2s_mode == I2S_MODE_SLAVE_RECEIVER) {
        CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_I2S2_SD_IN);
        CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_I2S2_WS_IN);
        CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_I2S2_CK_IN);
    }
}

#if 0

#ifdef CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE
#undef CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE
#endif
#define CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE 4096

#endif

static cyg_uint8 bus2_bbuf0 [CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE]
  __attribute__((aligned (2), section (".sram"))) = { 0 };
static cyg_uint8 bus2_bbuf1 [CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE]
  __attribute__((aligned (2), section (".sram"))) = { 0 };

static const cyg_i2s_cortexm_stm32_bus_setup_t i2s_bus2_setup = {
  .i2s_clk              = &hal_stm32_sysclk,
  .i2s_reg_base         = CYGHWR_HAL_STM32_SPI2,
  .i2s_enable           = CYGHWR_HAL_STM32_SPI2_CLOCK,

  .i2s_pin_set          = stm32_i2s_bus2_pin_setup,

  .dma_tx_intr_pri      = CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_DMA_TXINTR_PRI,
  .dma_rx_intr_pri      = CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_DMA_RXINTR_PRI,

#if (CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE > 0)
  .bbuf_size            = CYGNUM_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_BBUF_SIZE,
  .bbuf0                = bus2_bbuf0,
  .bbuf1                = bus2_bbuf1,
#else
  .bbuf_size            = 0,
#endif

  .mck_enable           = CYGDAT_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_MCK_ENABLE,
};

static cyg_i2s_param_conf_t stm32_i2s_bus2_conf = {
    .i2s_audio_freq = I2S_AUDIO_FREQ_16K,
    .i2s_mode       = I2S_MODE_MASTER_SENDER,
    .i2s_standard   = I2S_STANDARD_PHILLIPS,
    .i2s_data_len   = I2S_DATA_LEN_16BITS,
    .i2s_ch_len     = I2S_CH_LEN_16BITS,
    .enable         = 0,
};

cyg_i2s_cortexm_stm32_bus_t cyg_i2s_stm32_bus2 = {
  .setup                    = &i2s_bus2_setup,

  .conf                     = &stm32_i2s_bus2_conf,

  .dma_tx_stream.desc       = CYGHWR_HAL_STM32_SPI2_DMA_TX,// The same as SPI
  .dma_tx_stream.callback   = stm32_dma_callback,
  .dma_tx_stream.data       = (CYG_ADDRWORD)&cyg_i2s_stm32_bus2,

  .dma_rx_stream.desc       = CYGHWR_HAL_STM32_SPI2_DMA_RX,// The same as SPI
  .dma_rx_stream.callback   = stm32_dma_callback,
  .dma_rx_stream.data       = (CYG_ADDRWORD)&cyg_i2s_stm32_bus2,
};
#endif /* CYGHWR_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2 */


#ifdef CYGHWR_DEVS_SOUND_I2S_CORTEXM_STM32_BUS3
// TODO: None
#endif /* CYGHWR_DEVS_SOUND_I2S_CORTEXM_STM32_BUS3 */


//-----------------------------------------------------------------------------

static void stm32_i2s_enable(cyg_i2s_cortexm_stm32_bus_t * stm32_bus)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    reg_addr = stm32_bus->setup->i2s_reg_base + CYGHWR_HAL_STM32_SPI_I2SCFGR;
    HAL_READ_UINT32 (reg_addr, reg_data);
    reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SE;
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    stm32_bus->conf->enable = 1;
}

static void stm32_i2s_disable(cyg_i2s_cortexm_stm32_bus_t * stm32_bus)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    reg_addr = stm32_bus->setup->i2s_reg_base + CYGHWR_HAL_STM32_SPI_I2SCFGR;
    HAL_READ_UINT32 (reg_addr, reg_data);
    reg_data &= (~(CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SE));
    HAL_WRITE_UINT32 (reg_addr, reg_data);

    stm32_bus->conf->enable = 0;
}

static void stm32_i2s_pr_conf(cyg_i2s_cortexm_stm32_bus_t * stm32_bus)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data = 0;
    cyg_uint32 tmp = 0;
    cyg_uint16 i2s_div = 2, i2s_odd = 0;
    cyg_uint8 data_len;

    reg_addr = stm32_bus->setup->i2s_reg_base + CYGHWR_HAL_STM32_SPI_I2SPR;
    if (stm32_bus->setup->mck_enable) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SPR_MCKOE;
    }

    /* Compute the Real divider depending on the MCLK output state with a floating point */
    if (stm32_bus->setup->mck_enable) {
        /* MCLK output is enabled */
        tmp = (((*stm32_bus->setup->i2s_clk) / 256) * 10 / \
                                stm32_bus->conf->i2s_audio_freq) + 5;
    } else {
        /* MCLK output is disabled */
        data_len = (stm32_bus->conf->i2s_data_len == I2S_DATA_LEN_16BITS ? 1 : 2);
        tmp = (((*stm32_bus->setup->i2s_clk) / (32 * data_len)) * 10 / \
                                stm32_bus->conf->i2s_audio_freq) + 5;
    }
    tmp = tmp / 10;/* Remove the floating point */

    /* Check the parity of the divider */
    i2s_odd = (cyg_uint16)(tmp & (cyg_uint16)0x0001);
    /* Compute the i2s_div prescaler */
    i2s_div = (cyg_uint16)((tmp - i2s_odd) / 2);
    /* Test if the divider is 1 or 0 or greater than 0xFF */
    if ((i2s_div < 2) || (i2s_div > 0xFF)) {
        /* Set the default values */
        i2s_div = 2;
        i2s_odd = 0;
    }

    reg_data |= CYGHWR_HAL_STM32_SPI_I2SPR_I2SDIV((i2s_div & 0xFF));
    if (i2s_odd) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SPR_ODD;
    }

    i2s_diag("i2s_div = %d, i2s_odd = %d\n", i2s_div, i2s_odd);
    HAL_WRITE_UINT32 (reg_addr, reg_data);
}


// Set up a new SPI bus on initialisation.
static void stm32_i2s_bus_conf(
        cyg_i2s_cortexm_stm32_bus_t * stm32_bus, const bool enable_i2s)
{
    cyg_haladdress reg_addr;
    cyg_uint32 reg_data;

    i2s_diag("i2s conf: audio_freq = %d, standard = %d, data_len = %d, mode = %d\n", \
            stm32_bus->conf->i2s_audio_freq, stm32_bus->conf->i2s_standard, \
            stm32_bus->conf->i2s_data_len, stm32_bus->conf->i2s_mode);

    if (stm32_bus->setup->i2s_pin_set) {
        stm32_bus->setup->i2s_pin_set(stm32_bus);
    }

    // Set up I2S/SPI default configuration.
    reg_addr = stm32_bus->setup->i2s_reg_base + CYGHWR_HAL_STM32_SPI_CR2;
    reg_data = CYGHWR_HAL_STM32_SPI_CR2_TXDMAEN | CYGHWR_HAL_STM32_SPI_CR2_RXDMAEN;
    HAL_WRITE_UINT32 (reg_addr, reg_data);


    /* I2S Configuration */

    reg_data = 0;
    reg_addr = stm32_bus->setup->i2s_reg_base + CYGHWR_HAL_STM32_SPI_I2SCFGR;
    stm32_i2s_disable(stm32_bus);// Disable I2S before setting
    // Channel length (number of bits per audio channel)
    if (stm32_bus->conf->i2s_ch_len == I2S_CH_LEN_32BITS) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_CHLEN;
    }
    // Data length to be transferred
    if (stm32_bus->conf->i2s_data_len == I2S_DATA_LEN_24BITS) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_DATLEN24;
    } else if (stm32_bus->conf->i2s_data_len == I2S_DATA_LEN_32BITS) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_DATLEN32;
    }

    // Steady state clock polarity
    // Default set to LOW

    // I2S standard selection and PCM frame synchronization (only for PCM standard)
    if (stm32_bus->conf->i2s_standard == I2S_STANDARD_MSB) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SSTDMSB;
    } else if (stm32_bus->conf->i2s_standard == I2S_STANDARD_LSB) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SSTDLSB;
    } else if (stm32_bus->conf->i2s_standard == I2S_STANDARD_PCM_LONG) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SSTDPCM;
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_PCMSYNC;
    } else if (stm32_bus->conf->i2s_standard == I2S_STANDARD_PCM_SHORT) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SSTDPCM;
    }

    // I2S configuration mode
    if (stm32_bus->conf->i2s_mode == I2S_MODE_SLAVE_RECEIVER) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SCFGSR;
    } else if (stm32_bus->conf->i2s_mode == I2S_MODE_MASTER_SENDER) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SCFGMT;
    } else if (stm32_bus->conf->i2s_mode == I2S_MODE_MASTER_RECEIVER) {
        reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2SCFGMR;
    }

    // I2S mode selection
    reg_data |= CYGHWR_HAL_STM32_SPI_I2SCFGR_I2MOD;
    HAL_WRITE_UINT32 (reg_addr, reg_data);


    /* I2S prescaler */
    stm32_i2s_pr_conf(stm32_bus);


    /* Enable I2S */
    if (enable_i2s) {
        stm32_i2s_enable(stm32_bus);
    }
}

static void stm32_i2s_bus_setup(cyg_i2s_cortexm_stm32_bus_t * stm32_bus)
{
    stm32_i2s_bus_conf(stm32_bus, 0);

    CYGHWR_HAL_STM32_CLOCK_ENABLE( stm32_bus->setup->i2s_enable );

    // Initialise the synchronisation primitivies.
    cyg_drv_mutex_init (&stm32_bus->mutex);
    cyg_drv_cond_init (&stm32_bus->condvar, &stm32_bus->mutex);

    cyg_mutex_init(&stm32_bus->wr_mutex);

    // Initialize DMA streams
    hal_stm32_dma_init(&stm32_bus->dma_tx_stream, stm32_bus->setup->dma_tx_intr_pri);
    hal_stm32_dma_init(&stm32_bus->dma_rx_stream, stm32_bus->setup->dma_rx_intr_pri);
}


//-----------------------------------------------------------------------------
// Set up a DMA channel.

static void dma_channel_setup(cyg_i2s_cortexm_stm32_bus_t * stm32_bus,
                      cyg_uint8* data_buf, cyg_uint32 count, cyg_bool configure)
{
    hal_stm32_dma_stream * stream = stm32_bus->is_tx ?
            &stm32_bus->dma_tx_stream : &stm32_bus->dma_rx_stream;

    if (configure)
    hal_stm32_dma_configure( stream,
                             16,
                             (data_buf==NULL),
                             0 );

    if( data_buf == NULL ) {
        data_buf = (cyg_uint8 *)&dma_null;
    }

    hal_stm32_dma_start( stream,
                         data_buf,
                         stm32_bus->setup->i2s_reg_base + CYGHWR_HAL_STM32_SPI_DR,
                         (count >> 1) );

//  hal_stm32_dma_show( stream );
}


//-----------------------------------------------------------------------------
// DMA Callbacks. The DMA driver has disabled the DMA channel and
// masked the interrupt condition, here we need to wake up the client
// thread.

static void stm32_dma_callback( hal_stm32_dma_stream *stream,
                                      cyg_uint32 count, CYG_ADDRWORD data )
{
    cyg_i2s_cortexm_stm32_bus_t* stm32_bus = (cyg_i2s_cortexm_stm32_bus_t*)data;

    cyg_drv_dsr_lock ();

    // Update done count
    stm32_bus->done_count = (stm32_bus->cur_tr_size - count);

    if (stm32_bus->next_tr_size == 0) {
        stm32_bus->dma_done = true;
    } else {
        stm32_bus->cur_tr_size = stm32_bus->next_tr_size;
        dma_channel_setup(stm32_bus,
                          (stm32_bus->bbuf_busy == 0 ? stm32_bus->setup->bbuf1 :
                                                       stm32_bus->setup->bbuf0),
                          stm32_bus->cur_tr_size,
                          0);

        // Update bbuf busy
        stm32_bus->bbuf_busy = (stm32_bus->bbuf_busy == 0 ? 1 : 0);
    }

    i2s_diag("done_count = %d, bbuf_busy = %d, cur_tr_size = %d, next_tr_size = %d\n",
              stm32_bus->done_count, stm32_bus->bbuf_busy,
              stm32_bus->cur_tr_size, stm32_bus->next_tr_size);

    cyg_drv_cond_signal (&stm32_bus->condvar);
    cyg_drv_dsr_unlock ();
}


//-----------------------------------------------------------------------------
// Initiate a DMA transfer over the I2S interface.

static void i2s_transaction_dma(cyg_i2s_cortexm_stm32_device_t * device,
                                     cyg_uint8 * const data, cyg_uint32 * len)
{
    cyg_i2s_cortexm_stm32_bus_t* stm32_bus = (cyg_i2s_cortexm_stm32_bus_t*)device->i2s_bus;
    cyg_uint32 offset = 0;
    cyg_uint8 * data_local;

    i2s_diag("device %p, data %p, len = %d\n", device, data, (*len));

    // Clear done flags prior to configuring DMA:
    stm32_bus->dma_done = false;
    stm32_bus->done_count= 0;

    // Transfer bbuf0 data firstly
    stm32_bus->bbuf_busy = 0;
    data_local = stm32_bus->setup->bbuf0;

    if ((*len) < stm32_bus->setup->bbuf_size) {
        stm32_bus->cur_tr_size  = (*len);
        stm32_bus->next_tr_size = 0;
    } else if ((*len) < (stm32_bus->setup->bbuf_size * 2)) {
        stm32_bus->cur_tr_size  = stm32_bus->setup->bbuf_size;
        stm32_bus->next_tr_size = (*len) - stm32_bus->setup->bbuf_size;
    } else {
        /* More than 2 * bbuf_size data to transfered */
        stm32_bus->cur_tr_size  = stm32_bus->setup->bbuf_size;
        stm32_bus->next_tr_size = stm32_bus->setup->bbuf_size;
    }

    if (stm32_bus->is_tx) {
        // Prepare data for two buff and update offset
        memcpy(stm32_bus->setup->bbuf0, data + offset, stm32_bus->cur_tr_size);
        offset += stm32_bus->cur_tr_size;
        memcpy(stm32_bus->setup->bbuf1, data + offset, stm32_bus->next_tr_size);
        offset += stm32_bus->next_tr_size;
    }
    i2s_diag("stm32_bus->cur_tr_size = %d, stm32_bus->next_tr_size = %d, offset = %d\n",
                    stm32_bus->cur_tr_size, stm32_bus->next_tr_size, offset);
    i2s_diag("************************************************************\n\n");

    // Set up the DMA channels.
    dma_channel_setup (stm32_bus, data_local, stm32_bus->cur_tr_size, 1);

    // Run the DMA (interrupt driven).
    cyg_drv_mutex_lock (&stm32_bus->mutex);
    cyg_drv_dsr_lock ();

    // Sit back and wait for the ISR/DSRs to signal completion.
    do {
        cyg_drv_cond_wait (&stm32_bus->condvar);
        i2s_diag("stm32_bus->done_count = %d, stm32_bus->bbuf_busy = %d, offset = %d\n",
                  stm32_bus->done_count, stm32_bus->bbuf_busy, offset);

        data_local = (stm32_bus->bbuf_busy == 0 ?
                      stm32_bus->setup->bbuf1 : stm32_bus->setup->bbuf0);

        if (stm32_bus->is_tx) {
            // Prepare next transfer data
            if ((int)((*len) - offset) <= 0) {
                stm32_bus->next_tr_size = 0;
            } else {
                if (((*len) - offset) < stm32_bus->setup->bbuf_size) {
                    stm32_bus->next_tr_size = (cyg_uint16)((*len) - offset);
                } else {
                    stm32_bus->next_tr_size = stm32_bus->setup->bbuf_size;
                }
                memcpy(data_local, data + offset, stm32_bus->next_tr_size);
                offset += stm32_bus->next_tr_size;
            }
        } else {
            memcpy(data + offset, data_local, stm32_bus->done_count);
            if ((int)((*len) - offset) <= 0) {
                stm32_bus->next_tr_size = 0;
            } else {
                offset += stm32_bus->done_count;
                if (((*len) - offset) < stm32_bus->setup->bbuf_size) {
                    stm32_bus->next_tr_size = (cyg_uint16)((*len) - offset);
                } else {
                    stm32_bus->next_tr_size = stm32_bus->setup->bbuf_size;
                }
            }
        }
        i2s_diag("stm32_bus->next_tr_size = %d\n", stm32_bus->next_tr_size);
        i2s_diag("--------------------------------------------------------\n");
    } while (!stm32_bus->dma_done);

    cyg_drv_dsr_unlock ();
    cyg_drv_mutex_unlock (&stm32_bus->mutex);
}


// ----------------------------------------------------------------------------

// Initialise I2S interfaces on startup.
static bool i2s_stm32_init(struct cyg_devtab_entry *tab)
{
    cyg_i2s_cortexm_stm32_device_t * stm32_device =
                    (cyg_i2s_cortexm_stm32_device_t *)tab->priv;

    stm32_i2s_bus_setup(stm32_device->i2s_bus);
    return true;
}

static
Cyg_ErrNo i2s_stm32_lookup(struct cyg_devtab_entry **tab,
                                struct cyg_devtab_entry *st,
                                const char *name)
{
    cyg_i2s_cortexm_stm32_device_t* stm32_device =
                    (cyg_i2s_cortexm_stm32_device_t *)(*tab)->priv;
    cyg_i2s_cortexm_stm32_bus_t* stm32_bus = stm32_device->i2s_bus;

    stm32_i2s_disable(stm32_bus);
    return ENOERR;
}

static
Cyg_ErrNo i2s_stm32_write(cyg_io_handle_t handle,
                             const void *buf,
                             cyg_uint32 *len)
{
    cyg_devtab_entry_t * t = (cyg_devtab_entry_t *)handle;
    cyg_i2s_cortexm_stm32_device_t* stm32_device =
                                (cyg_i2s_cortexm_stm32_device_t*)t->priv;
    cyg_i2s_cortexm_stm32_bus_t* stm32_bus = stm32_device->i2s_bus;

    // Check for unsupported transactions.
    CYG_ASSERT ((*len) > 0, "STM32 I2S : Null transfer requested.");

    i2s_diag("len %d\n", (*len));
    if (buf) i2s_dump_buf(buf, (*len));

    // We check that the buffers are half-word aligned and that count is a
    // multiple of two in order to carry out the 16-bit transfer.
    CYG_ASSERT (!((*len) & 1) && !((cyg_uint32)buf & 1),
                            "STM32 I2S : Misaligned data in 16-bit transfer.");

    cyg_mutex_lock(&stm32_bus->wr_mutex);

    // Set transmission direction
    stm32_bus->is_tx = 1;

    // Perform transfer via the bounce buffers.
    if (stm32_bus->setup->bbuf_size != 0) {
        i2s_transaction_dma(stm32_device, (cyg_uint8 *)buf, len);
    } else {
        *len = 0;
    }

    cyg_mutex_unlock(&stm32_bus->wr_mutex);
    i2s_diag("done\n");
    return ENOERR;
}

static
Cyg_ErrNo i2s_stm32_read(cyg_io_handle_t handle,
                             void *buffer,
                             cyg_uint32 *len)
{
    cyg_devtab_entry_t * t = (cyg_devtab_entry_t *)handle;
    cyg_i2s_cortexm_stm32_device_t* stm32_device = (cyg_i2s_cortexm_stm32_device_t*)t->priv;
    cyg_i2s_cortexm_stm32_bus_t* stm32_bus = stm32_device->i2s_bus;

    // Check for unsupported transactions.
    CYG_ASSERT ((*len) > 0, "STM32 I2S : Null transfer requested.");

    i2s_diag("len %d\n", (*len));

    // We check that the buffers are half-word aligned and that count is a
    // multiple of two in order to carry out the 16-bit transfer.
    CYG_ASSERT (!((*len) & 1) && !((cyg_uint32)buffer & 1),
                            "STM32 I2S : Misaligned data in 16-bit transfer.");

    cyg_mutex_lock(&stm32_bus->wr_mutex);

    // Set transmission direction
    stm32_bus->is_tx = 0;

    // Perform transfer via the bounce buffers.
    if (stm32_bus->setup->bbuf_size != 0) {
        i2s_diag("Clear DMA buffer.\n");
        memset(stm32_bus->setup->bbuf0, 0, stm32_bus->setup->bbuf_size);
        memset(stm32_bus->setup->bbuf1, 0, stm32_bus->setup->bbuf_size);

        i2s_transaction_dma(stm32_device, buffer, len);
        i2s_dump_buf(buffer, (*len) );
//        diag_dump_buf( buffer, 1024 * 16 );
    } else {
        *len = 0;
    }

    cyg_mutex_unlock(&stm32_bus->wr_mutex);
    i2s_diag("done\n");
    return ENOERR;
}


static
Cyg_ErrNo i2s_stm32_set_config(cyg_io_handle_t handle,
                                    cyg_uint32 key,
                                    const void *buffer,
                                    cyg_uint32 *len)
{
    cyg_devtab_entry_t * t = (cyg_devtab_entry_t *)handle;
    cyg_i2s_cortexm_stm32_device_t* stm32_device = (cyg_i2s_cortexm_stm32_device_t*)t->priv;
    cyg_i2s_cortexm_stm32_bus_t * stm32_bus = stm32_device->i2s_bus;

    switch (key) {
    case CYG_IO_SET_CONFIG_I2S_SWITCH:
        if (((cyg_uint32)*((cyg_uint32 *)buffer))) {
            stm32_i2s_enable(stm32_bus);
        } else {
            stm32_i2s_disable(stm32_bus);
        }
        return  ENOERR;
        break;

    case CYG_IO_SET_CONFIG_I2S_AUDIO_FREQ:
        stm32_bus->conf->i2s_audio_freq = (cyg_uint32)(*((cyg_uint32 *)buffer));
        break;

    case CYG_IO_SET_CONFIG_I2S_STANDARD:
        stm32_bus->conf->i2s_standard = (cyg_uint8)(*((cyg_uint32 *)buffer));
        break;

    case CYG_IO_SET_CONFIG_I2S_DATA_LEN:
        stm32_bus->conf->i2s_data_len = (cyg_uint8)(*((cyg_uint32 *)buffer));
        if (stm32_bus->conf->i2s_data_len == I2S_DATA_LEN_24BITS ||
            stm32_bus->conf->i2s_data_len == I2S_DATA_LEN_32BITS) {
            stm32_bus->conf->i2s_ch_len = I2S_CH_LEN_32BITS;
        }
        break;

    case CYG_IO_SET_CONFIG_I2S_MODE:
        stm32_bus->conf->i2s_mode = (cyg_uint8)(*((cyg_uint32 *)buffer));
        i2s_diag("stm32_bus->conf->i2s_mode = %d\n", stm32_bus->conf->i2s_mode);
        break;

    default:
        return -EINVAL;
    }

    stm32_i2s_bus_conf(stm32_bus, stm32_bus->conf->enable);

    return ENOERR;
}

static
Cyg_ErrNo i2s_stm32_get_config(cyg_io_handle_t handle,
                                    cyg_uint32 key,
                                    void *buffer,
                                    cyg_uint32 *len)
{
    return -ENOTSUP;
//  return ENOERR;
}


// -----------------------------------------------------------------------------
// End of file
