//==========================================================================
//
//      i2s_stm32.h
//
//      I2S driver for STM32 CortexM processors
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
// Date:          2013-06-02
// Description:   I2S bus driver for STM32
//####DESCRIPTIONEND####
//==========================================================================

#ifndef CYGONCE_DEVS_SOUND_I2S_CORTEXM_STM32_H
#define CYGONCE_DEVS_SOUND_I2S_CORTEXM_STM32_H

#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/var_dma.h>


// ----------------------------------------------------------------------------
// Get/Set configuration 'key' values for I2S interface.

#define CYG_IO_SET_CONFIG_I2S_SWITCH            0x11000001

#define CYG_IO_SET_CONFIG_I2S_AUDIO_FREQ        0x11000002
#define CYG_IO_SET_CONFIG_I2S_STANDARD          0x11000003
#define CYG_IO_SET_CONFIG_I2S_DATA_LEN          0x11000004
#define CYG_IO_SET_CONFIG_I2S_MODE              0x11000005




//-----------------------------------------------------------------------------
// STM32 SPI bus configuration and state.

typedef void i2s_pin_set_func(void * _stm32_bus_);

typedef struct {
    cyg_uint32        *i2s_clk;        // I2S bus clk source.
    cyg_haladdress    i2s_reg_base;    // Base address of I2S register block.
    cyg_uint32        i2s_enable;      // I2S bus clock enable

    i2s_pin_set_func  *i2s_pin_set;    // Function for setting pin.

    cyg_priority_t    dma_tx_intr_pri; // Interrupt priority for DMA transmit.
    cyg_priority_t    dma_rx_intr_pri; // Interrupt priority for DMA receive.

    cyg_uint32        bbuf_size;       // Size of bounce buffers.
    cyg_uint8*        bbuf0;           // Pointer to bounce buffer.
    cyg_uint8*        bbuf1;           // Pointer to bounce buffer.

    cyg_uint8         mck_enable;      // Master clock output enable.
} cyg_i2s_cortexm_stm32_bus_setup_t;


// ----------------------------------------------------------------------------
// I2S Configuration

typedef struct {
    cyg_uint32        i2s_audio_freq;  // I2S audio frequency.

    cyg_uint8         i2s_mode;        // I2S configuration mode, see follow.
    cyg_uint8         i2s_standard;    // I2S standard selection, see follow.
    cyg_uint8         i2s_data_len;    // I2S data length to be transferred, see follow.
    cyg_uint8         i2s_ch_len;      // I2S channel length (number of bits per audio channel).

    cyg_bool          enable;          // Enable I2S.
} cyg_i2s_param_conf_t;

/* I2S audio frequency */
#define I2S_AUDIO_FREQ_192K             192000
#define I2S_AUDIO_FREQ_96K              96000
#define I2S_AUDIO_FREQ_48K              48000
#define I2S_AUDIO_FREQ_44K              44100
#define I2S_AUDIO_FREQ_32K              32000
#define I2S_AUDIO_FREQ_22K              22050
#define I2S_AUDIO_FREQ_16K              16000
#define I2S_AUDIO_FREQ_11K              11025
#define I2S_AUDIO_FREQ_8K               8000

/* I2S configuration mode */
#define I2S_MODE_SLAVE_SENDER           0x00
#define I2S_MODE_SLAVE_RECEIVER         0x01
#define I2S_MODE_MASTER_SENDER          0x02
#define I2S_MODE_MASTER_RECEIVER        0x03

/* I2S standard selection */
#define I2S_STANDARD_PHILLIPS           0x00
#define I2S_STANDARD_MSB                0x01
#define I2S_STANDARD_LSB                0x02
# define I2S_STANDARD_PCM_SHORT         0x03
# define I2S_STANDARD_PCM_LONG          0x04

/* I2S data length to be transferred */
#define I2S_DATA_LEN_16BITS             0
#define I2S_DATA_LEN_24BITS             1
#define I2S_DATA_LEN_32BITS             2

/* I2S channel length */
#define I2S_CH_LEN_16BITS               0
#define I2S_CH_LEN_32BITS               1


typedef struct {
    // ---- Bus configuration constants ----
    const cyg_i2s_cortexm_stm32_bus_setup_t * setup;

    cyg_i2s_param_conf_t * conf;

    // ---- Driver state (private) ----
    hal_stm32_dma_stream dma_tx_stream;// TX DMA stream for this bus.
    hal_stm32_dma_stream dma_rx_stream;// RX DMA stream for this bus.
    cyg_drv_mutex_t      mutex;        // Transfer mutex.
    cyg_drv_cond_t       condvar;      // Transfer condition variable.
    cyg_bool             dma_done;     // Flags used to signal completion.

    // ---- Internal private ----
    cyg_mutex_t          wr_mutex;     // Transfer mutex for write and read.
    cyg_bool             is_tx;

    /* Run time data */
    cyg_uint32           done_count;   //
    cyg_uint16           cur_tr_size;
    cyg_uint16           next_tr_size;
    cyg_uint8            bbuf_busy;    // 0: bbuf is busy, 1: bbuf0 is busy.
} cyg_i2s_cortexm_stm32_bus_t;


//-----------------------------------------------------------------------------
// STM32 I2S device.

typedef struct {
    cyg_i2s_cortexm_stm32_bus_t * i2s_bus;
} cyg_i2s_cortexm_stm32_device_t;


#endif // #endif CYGONCE_DEVS_SOUND_I2S_CORTEXM_STM32_H

// ----------------------------------------------------------------------------
// EOF i2c_stm32.h
