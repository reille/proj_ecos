//==========================================================================
//
//      wm8978.h
//
//      WM8978 driver for WOLFSON stereo codec with speaker driver
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
// Description:   WM8978 driver for WOLFSON stereo codec with speaker driver
//####DESCRIPTIONEND####
//==========================================================================

#ifndef CYGONCE_DEVS_SOUND_I2C_WM8978_H
#define CYGONCE_DEVS_SOUND_I2C_WM8978_H

#include <pkgconf/hal.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/i2c.h>
#include <cyg/hal/var_dma.h>


// ----------------------------------------------------------------------------
// Get/Set configuration 'key' values for sound card.
// This is used to upper level.

#define CYG_IO_SET_CONFIG_SND_CARD_SWITCH       0x10000001
#define CYG_IO_SET_CONFIG_SND_CARD_MUTE         0x10000002

#define CYG_IO_SET_CONFIG_SND_CARD_AUDIO_FREQ   0x10000011
#define CYG_IO_SET_CONFIG_SND_CARD_STANDARD     0x10000012
#define CYG_IO_SET_CONFIG_SND_CARD_DATA_LEN     0x10000013

#define CYG_IO_SET_CONFIG_SND_CARD_VOLUME       0x10000021
#define CYG_IO_SET_CONFIG_SND_CARD_MIC_GAIN     0x10000022

#define CYG_IO_SET_CONFIG_SND_CARD_ROLE         0x10000031/* Sound card input */
#define CYG_IO_SET_CONFIG_SND_CARD_OUTPUT       0x10000032/* Sound card output */

#define CYG_IO_SET_CONFIG_SND_CARD_CHANNEL      0x10000041



// Get/Set configuration 'key' values for I2S.
// Interval.

#define CYG_IO_SET_CONFIG_I2S_SWITCH            0x11000001

#define CYG_IO_SET_CONFIG_I2S_AUDIO_FREQ        0x11000002
#define CYG_IO_SET_CONFIG_I2S_STANDARD          0x11000003
#define CYG_IO_SET_CONFIG_I2S_DATA_LEN          0x11000004
#define CYG_IO_SET_CONFIG_I2S_MODE              0x11000005






// ----------------------------------------------------------------------------
// Set sound and audio card param.

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

/* I2S configuration mode */
#define I2S_MODE_SLAVE_SENDER           0x00
#define I2S_MODE_SLAVE_RECEIVER         0x01
#define I2S_MODE_MASTER_SENDER          0x02
#define I2S_MODE_MASTER_RECEIVER        0x03


#define SOUND_CARD_PARAM_OUTPUT_NONE    0x00
#define SOUND_CARD_PARAM_OUTPUT_SPEAKER 0x01
#define SOUND_CARD_PARAM_OUTPUT_EAR     0x02

#define SOUND_CARD_PARAM_ROLE_PLAYER    0x00
#define SOUND_CARD_PARAM_ROLE_RECORD    0x01
#define SOUND_CARD_PARAM_ROLE_AUX       0x02

/* Sound channel set */
#define SND_CARD_PARAM_CHANNEL_STEREO   0x00
#define SND_CARD_PARAM_CHANNEL_MONO_L_CH 0x01
#define SND_CARD_PARAM_CHANNEL_MONO_R_CH 0x02




typedef struct {
    cyg_uint16          left_volume;
    cyg_uint16          right_volume;
} cyg_snd_card_volume_t;



// ----------------------------------------------------------------------------
// Some macro

#define DAC_ON			1
#define DAC_OFF			0

#define AUX_ON			1
#define AUX_OFF			0

#define LINE_ON			1
#define LINE_OFF		0

#define EAR_ON			1
#define EAR_OFF			0

#define SPK_ON			1
#define SPK_OFF			0

#define MIC_ON			1
#define MIC_OFF			0


/* 定义最大音量 */
#define WM8978_VOLUME_MAX		63		/* 最大音量 */
#define WM8978_VOLUME_STEP		3		/* 音量调节步长 */

/* 定义最大MIC增益 */
#define WM8978_GAIN_MAX		    63		/* 最大增益 */
#define WM8978_GAIN_STEP		3		/* 增益步长 */


// ----------------------------------------------------------------------------
// wm8978 device

typedef struct {
    cyg_i2c_device *            i2c_dev;
    cyg_io_handle_t             dev_handle;/* dependent i2s device handle */

    cyg_uint8                   role;
    cyg_uint8                   output;

    cyg_uint8                   init;
} snd_wm8978_dev_t;


#endif // #endif CYGONCE_DEVS_SOUND_I2C_WM8978_H

// ----------------------------------------------------------------------------
// EOF wm8978.h
