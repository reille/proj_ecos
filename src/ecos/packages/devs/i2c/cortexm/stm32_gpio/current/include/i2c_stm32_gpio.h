//==========================================================================
//
//      devs/i2c/cortexm/stm32/current/src/i2c_stm32_gpio.h
//
//      GPIO-based bitbanging I2C driver for STM32 CortexM processors
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
// Date:          2013-05-25
// Description:   GPIO-based bitbanging I2C driver for STM32
//####DESCRIPTIONEND####
//==========================================================================

#ifndef CYGONCE_I2C_STM32_GPIO_H
#define CYGONCE_I2C_STM32_GPIO_H


#include <cyg/infra/cyg_type.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/i2c.h>

//
// Internal STM32 GPIO-based bitbanging I2C functions
//

externC void        cyg_i2c_stm32_gpio_init(struct cyg_i2c_bus*);

// These function MUST be called within a thread.
externC cyg_uint32  cyg_i2c_stm32_gpio_tx(const cyg_i2c_device*,
                            cyg_bool, const cyg_uint8*, cyg_uint32, cyg_bool);
externC cyg_uint32  cyg_i2c_stm32_gpio_rx(const cyg_i2c_device*,
                            cyg_bool, cyg_uint8*, cyg_uint32, cyg_bool, cyg_bool);
externC void        cyg_i2c_stm32_gpio_stop(const cyg_i2c_device*);

typedef struct cyg_i2c_stm32_gpio_extra {
    cyg_uint8        i2c_wait; // Maximum time to wait for the i2c device to respond
} cyg_i2c_stm32_gpio_extra;


//
// Functions/macros for the host usage.
//


// Macro for initialising an I2C device.
#define CYG_I2C_STM32_DEVICE(_name_,_address_)                       \
    /* Default to maximum responds time 100 ms */                    \
    static cyg_i2c_stm32_gpio_extra _name_ ## _extra = {.i2c_wait = 100};  \
    static CYG_I2C_BUS(_name_ ## _stm32_bus,                         \
                       cyg_i2c_stm32_gpio_init,                      \
                       cyg_i2c_stm32_gpio_tx,                        \
                       cyg_i2c_stm32_gpio_rx,                        \
                       cyg_i2c_stm32_gpio_stop,                      \
                       (void *)&_name_ ## _extra);                   \
    CYG_I2C_DEVICE(_name_,                                           \
                   &_name_ ## _stm32_bus,                            \
                   _address_,                                        \
                   0x0,                                              \
                   10000)


#endif // #endif CYGONCE_I2C_STM32_GPIO_H

// -------------------------------------------------------------------------
// EOF i2c_stm32_gpio.h
