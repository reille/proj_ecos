//==========================================================================
//
//      wm8978.inl
//
//      WM8978 driver for WOLFSON stereo codec with speaker driver
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2008 Free Software Foundation, Inc.
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
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================

#ifndef CYGONCE_DEVS_SOUND_I2C_WM8978_INL
#define CYGONCE_DEVS_SOUND_I2C_WM8978_INL

#include <cyg/io/devtab.h>


// -------------------------------------------------------------------------
// Driver private data


#include <cyg/io/i2c_stm32_gpio.h>
CYG_I2C_STM32_DEVICE(i2c_wm8978, 0x1A);


static snd_wm8978_dev_t snd_wm8978_dev = {
    .i2c_dev = &i2c_wm8978,
};



// -------------------------------------------------------------------------
// Device instance

CHAR_DEVIO_TABLE(snd_wm8978_handlers,
        snd_wm8978_write,
        snd_wm8978_read,
        NULL,                       // Unsupport select
        snd_wm8978_get_config,
        snd_wm8978_set_config);

CHAR_DEVTAB_ENTRY(snd_wm8978_device,
        CYGDAT_DEVS_SOUND_I2C_WM8978_NAME,
        CYGDAT_DEVS_SOUND_I2C_WM8978_USE_I2S_DEV,// I2S channel device name
        &snd_wm8978_handlers,
        snd_wm8978_init,
        snd_wm8978_lookup,
        (void *)&snd_wm8978_dev);   // Private data pointer


#endif // CYGONCE_DEVS_SOUND_I2C_WM8978_INL


// ---------------------------------------------------------------------------
// End of file
