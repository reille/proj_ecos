//==========================================================================
//
//      i2s_stm32.inl
//
//      I2S drivers for the stm32
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

#ifndef CYGONCE_DEVS_SOUND_I2S_CORTEXM_STM32_INL
#define CYGONCE_DEVS_SOUND_I2S_CORTEXM_STM32_INL

#include <cyg/io/devtab.h>


// -------------------------------------------------------------------------
// Driver private data

externC cyg_i2s_cortexm_stm32_bus_t cyg_i2s_stm32_bus2;

static cyg_i2s_cortexm_stm32_device_t i2s_stm32_bus2_dev = {
    .i2s_bus = &cyg_i2s_stm32_bus2,
};


// -------------------------------------------------------------------------
// I2S device IO

CHAR_DEVIO_TABLE(cyg_io_i2s_cortexm_stm32_devio,
                 i2s_stm32_write,
                 i2s_stm32_read,
                 NULL,                                  // Unsupport select
                 i2s_stm32_get_config,
                 i2s_stm32_set_config);


// -------------------------------------------------------------------------
// Device instance

CHAR_DEVTAB_ENTRY(stm32_i2s_bus2_device,
                  CYGDAT_DEVS_SOUND_I2S_CORTEXM_STM32_BUS2_NAME,
                  NULL,                                 // Base device name
                  &cyg_io_i2s_cortexm_stm32_devio,
                  i2s_stm32_init,
                  i2s_stm32_lookup,
                  (void *)&i2s_stm32_bus2_dev);       // Private data pointer


#endif // CYGONCE_DEVS_SOUND_I2S_CORTEXM_STM32_INL


// ---------------------------------------------------------------------------
// End of file
