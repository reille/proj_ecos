#ifndef CYGONCE_DEVS_TOUCH_SPI_TSC2046_H
#define CYGONCE_DEVS_TOUCH_SPI_TSC2046_H

//=============================================================================
//
//      tsc2046.h
//
//      Drivers for the TSC2046 touch screen controller
//
//=============================================================================
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
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   ccoutand
// Date:        2012-01-05
// Purpose:     Touchscreen drivers for the TSC20460
//
//####DESCRIPTIONEND####
//
//=============================================================================

typedef int (*get_pen_state_fn)( void );

struct cyg_touch_spi_tsc2046_dev {
  const void                      *priv;                  // Devices private data
  get_pen_state_fn                get_pen_state;

};

#define CYG_DEVS_TOUCH_SPI_TSC2046_DRIVER(_spidev_, _fn_ )   \
static struct cyg_touch_spi_tsc2046_dev ts_dev =             \
{                                                            \
    .priv               = _spidev_,                          \
    .get_pen_state      = _fn_                               \
}

//-----------------------------------------------------------------------------
#endif // CYGONCE_DEVS_TOUCH_SPI_TSC2046_H
// EOF tsc2046.h
