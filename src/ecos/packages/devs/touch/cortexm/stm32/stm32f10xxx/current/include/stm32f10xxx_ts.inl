//==========================================================================
//
//      stm32f10xxx_ts.inl
//
//      Touch screen - Hardware support for the STM32F10XXX board
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2012 Free Software Foundation, Inc.
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
// Date:         2013-05-04
// Purpose:
// Description:
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>
#include <cyg/hal/var_io.h>

// ------------------------------------------------------------------------
// There is also an TSC2046 Touch screen controller on the SPI bus

#if defined(CYGPKG_IO_SPI) && defined(CYGPKG_DEVS_TOUCH_SPI_TSC2046)

#define CYGHWR_HAL_STM32F10XXX_TOUCH_PEN_IRQ  CYGHWR_HAL_STM32_GPIO( C, 5,  IN, PULLUP )

static int stm32f10xxx_get_pen_down( void ){
   int state;
   hal_stm32_gpio_in(CYGHWR_HAL_STM32F10XXX_TOUCH_PEN_IRQ, &state);
   return state;
}

#include <cyg/io/spi.h>
#include <cyg/io/spi_stm32.h>
#include <cyg/io/tsc2046.h>

CYG_DEVS_SPI_CORTEXM_STM32_DEVICE (
    tsc2046_spi_device, 1, 1, false, 0, 0, 1000000, 1, 1, 1
);

//-----------------------------------------------------------------------------
// Instantiate the TSC2046 device driver.

CYG_DEVS_TOUCH_SPI_TSC2046_DRIVER (
    &tsc2046_spi_device,
    stm32f10xxx_get_pen_down
);

#endif

// ------------------------------------------------------------------------
// EOF stm32f10xxx_ts.c
