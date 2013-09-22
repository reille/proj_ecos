//==========================================================================
//
//      tsc2046.inl
//
//      Touch screen drivers for the TSC2046
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
// Author(s):    ccoutand
// Contributors:
// Date:         2012-01-18
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================

#ifndef CYGONCE_DEVS_TOUCH_SPI_TSC2046_INL
#define CYGONCE_DEVS_TOUCH_SPI_TSC2046_INL

// -------------------------------------------------------------------------
// Device instance

CHAR_DEVIO_TABLE(tsc2046_ts_handlers,
                 NULL,                                   // Unsupported write() function
                 ts_read,
                 ts_select,
                 ts_get_config,
                 ts_set_config);

CHAR_DEVTAB_ENTRY(tsc2046_ts_device,
                  CYGDAT_DEVS_TOUCH_SPI_TSC2046_NAME,
                  NULL,                                   // Base device name
                  &tsc2046_ts_handlers,
                  ts_init,
                  ts_lookup,
                  NULL);                                  // Private data pointer

#include CYGDAT_DEVS_TOUCH_SPI_TSC2046_PLF_INL            // Include platform specifics

#endif // CYGONCE_DEVS_TOUCH_SPI_TSC2046_INL
