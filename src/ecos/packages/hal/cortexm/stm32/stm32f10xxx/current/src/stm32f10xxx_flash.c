/*==========================================================================
//
//      stm32f10xxx_flash.c
//
//      Cortex-M3 STM32F10XXX Flash setup
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
// Author(s):    nickg
// Date:         2008-07-30
// Description:
//
// Rev:          reille
// Date:         2013.02.22
//
//####DESCRIPTIONEND####
//
//========================================================================*/

#include <pkgconf/hal.h>
#include <pkgconf/hal_cortexm.h>
#include <pkgconf/hal_cortexm_stm32.h>
#include <pkgconf/hal_cortexm_stm32_stm32f10xxx.h>
#ifdef CYGPKG_KERNEL
#include <pkgconf/kernel.h>
#endif

#include <cyg/io/flash.h>
#include <cyg/io/flash_dev.h>
#include <cyg/hal/hal_io.h>


#ifdef CYGHWR_HAL_CORTEXM_STM32_FLASH_INTERNAL
//--------------------------------------------------------------------------
// Internal flash

#include <cyg/io/stm32_flash.h>

const cyg_stm32_flash_dev hal_stm32_flash_priv;

CYG_FLASH_DRIVER(hal_stm32_flash,
                 &cyg_stm32_flash_funs,
                 0,
                 0x08000000,
                 0,
                 0,
                 0,
                 &hal_stm32_flash_priv
);

#endif // CYGHWR_HAL_CORTEXM_STM32_FLASH_INTERNAL

#ifdef CYGHWR_HAL_CORTEXM_STM32_FLASH_NOR
//--------------------------------------------------------------------------
// There is a Spansion S29GL128P90FFIR20 or a NUMONYX equivalent.
// These are AMD compatible and with CFI can all be handled by the AMD
// driver.


#include <cyg/io/am29xxxxx_dev.h>

static const CYG_FLASH_FUNS(hal_stm3210e_flash_amd_funs,
                            &cyg_am29xxxxx_init_cfi_16,
                            &cyg_flash_devfn_query_nop,
                            &cyg_am29xxxxx_erase_16,
                            &cyg_am29xxxxx_program_16,
                            (int (*)(struct cyg_flash_dev*, const cyg_flashaddr_t, void*, size_t))0,
                            &cyg_flash_devfn_lock_nop,
                            &cyg_flash_devfn_unlock_nop);

static const cyg_am29xxxxx_dev hal_stm3210e_flash_priv;

CYG_FLASH_DRIVER(hal_stm3210e_flash,
                 &hal_stm3210e_flash_amd_funs,
                 0,
                 0x64000000,
                 0,
                 0,
                 hal_stm3210e_flash_priv.block_info,
                 &hal_stm3210e_flash_priv);

#endif // CYGHWR_HAL_CORTEXM_STM32_FLASH_NOR


# ifndef CYGPKG_REDBOOT // Added by reille 2013.06.12

// Add SPI FLASH | added by reille 2013.05.26
#ifdef CYGPKG_DEVS_FLASH_SPI_SST25XX
#include <cyg/io/sst25xx.h>

//-----------------------------------------------------------------------------
// Set up the SPI intertface on the ST STM32F10XXX board for the STM32.  The
// SST25XX device is connected to SPI bus 1. SPI1 should be set up in configtool
// so that chip select 1 corresponds to GPIO number 18 (PB2) and 50MHz I/O is
// enabled.  If the data area is in external RAM, SPI1 must be configured with
// bounce buffers of at least 256 bytes and the SST25XX read data block size
// should be set to the same size as the bounce buffers.

#ifdef CYGPKG_HAL_CORTEXM_STM32_STM32F10XXX
#include <cyg/io/spi_stm32.h>

CYG_DEVS_SPI_CORTEXM_STM32_DEVICE (
    sst25xx_spi_device, 1, 0, false, 0, 0, 25000000, 1, 1, 1
);

#endif

//-----------------------------------------------------------------------------
// Instantiate the SST25XX device driver.

CYG_DEVS_FLASH_SPI_SST25XX_DRIVER (
    sst25xx_flash_device, 0x65000000, &sst25xx_spi_device
);


#endif  /* CYGPKG_DEVS_FLASH_SPI_SST25XX */



// Add SPI FLASH | added by reille 2013.05.26
#ifdef CYGPKG_DEVS_FLASH_SPI_M25PXX
#include <cyg/io/m25pxx.h>

//-----------------------------------------------------------------------------
// Set up the SPI intertface on the ST STM32F10XXX board for the STM32.  The
// M25PXX device is connected to SPI bus 1. SPI1 should be set up in configtool
// so that chip select 1 corresponds to GPIO number 18 (PB2) and 50MHz I/O is
// enabled.  If the data area is in external RAM, SPI1 must be configured with
// bounce buffers of at least 256 bytes and the M25PXX read data block size
// should be set to the same size as the bounce buffers.

#ifdef CYGPKG_HAL_CORTEXM_STM32_STM32F10XXX
#include <cyg/io/spi_stm32.h>

CYG_DEVS_SPI_CORTEXM_STM32_DEVICE (
    m25pxx_spi_device, 1, 0, false, 0, 0, 25000000, 1, 1, 1
);

#endif

//-----------------------------------------------------------------------------
// Instantiate the M25Pxx device driver.

CYG_DEVS_FLASH_SPI_M25PXX_DRIVER (
    m25pxx_flash_device, 0x65000000, &m25pxx_spi_device
);

#endif  /* CYGPKG_DEVS_FLASH_SPI_M25PXX */

#endif  /* CYGPKG_REDBOOT */


//--------------------------------------------------------------------------
// EOF stm3210e_eval_flash.c
