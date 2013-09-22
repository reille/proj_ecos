//==========================================================================
//
//      stm32fb_init.cxx
//
//      Instantiate one or more framebuffer devices for the stm32f10xxx target.
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2008, 2009 Free Software Foundation, Inc.
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
//###DESCRIPTIONBEGIN####
//
// Author(s):     reille
// Date:          2013-04-28
//
//###DESCRIPTIONEND####
//========================================================================

#include <pkgconf/devs_framebuf_stm32.h>
#include <cyg/io/framebuf.h>

extern "C" void _cyg_stm32_fb_instantiate(struct cyg_fb *);

class _stm32_fb_init {

  public:
    _stm32_fb_init(void)
    {
#ifdef CYGPKG_DEVS_FRAMEBUF_STM32_FB0
        _cyg_stm32_fb_instantiate(&cyg_stm32_fb0);
#endif
    }
};

static _stm32_fb_init _stm32_fb_init_object CYGBLD_ATTRIB_INIT_PRI(CYG_INIT_DEV_CHAR);

//-------------------------------------------------------------------------
// End of file
