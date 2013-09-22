/*=============================================================================
//
//      stm32f10xxx_ts_test.c
//
//      Test case - Hardware support for the STM32F10XXX board
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
// Author(s):    reille
// Date:         2013-05-04
// Purpose:
// Description:  Test case - Hardware support for the STM32F10XXX board
//
//####DESCRIPTIONEND####
//
//===========================================================================*/

#include <pkgconf/system.h>
#include <pkgconf/hal.h>

#if defined(CYGPKG_KERNEL)
#include <pkgconf/kernel.h>
#endif

#include <cyg/infra/testcase.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

//=============================================================================
// Check all required packages and components are present

#if !defined(CYGPKG_KERNEL) || !defined(CYGPKG_KERNEL_API)

#define NA_MSG  "Configuration insufficient"

#endif

//=============================================================================
// If everything is present, compile the full test.

#ifndef NA_MSG

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <string.h>


//=============================================================================
// Access framebuffer

#ifdef CYGPKG_DEVS_FRAMEBUF_CORTEXM_STM32F10XXX

#include <pkgconf/io_framebuf.h>
#include <cyg/io/framebuf.h>

#if defined(CYGDAT_IO_FRAMEBUF_TEST_DEVICE)
# define FRAMEBUF   CYGDAT_IO_FRAMEBUF_TEST_DEVICE
#elif defined(CYGDAT_IO_FRAMEBUF_DEFAULT_TEST_DEVICE)
# define FRAMEBUF   CYGDAT_IO_FRAMEBUF_DEFAULT_TEST_DEVICE
#else
# define NA_MSG "No framebuffer test device selected"
#endif

#define STRING1(_a_) # _a_
#define STRING(_a_) STRING1(_a_)

// The colours used by this test code. Default to the standard palette
// but if running on a true colour display then adjust.
static cyg_ucount32 colours[16]  = {
    CYG_FB_DEFAULT_PALETTE_BLACK,
    CYG_FB_DEFAULT_PALETTE_BLUE,
    CYG_FB_DEFAULT_PALETTE_GREEN,
    CYG_FB_DEFAULT_PALETTE_CYAN,
    CYG_FB_DEFAULT_PALETTE_RED,
    CYG_FB_DEFAULT_PALETTE_MAGENTA,
    CYG_FB_DEFAULT_PALETTE_BROWN,
    CYG_FB_DEFAULT_PALETTE_LIGHTGREY,
    CYG_FB_DEFAULT_PALETTE_DARKGREY,
    CYG_FB_DEFAULT_PALETTE_LIGHTBLUE,
    CYG_FB_DEFAULT_PALETTE_LIGHTGREEN,
    CYG_FB_DEFAULT_PALETTE_LIGHTCYAN,
    CYG_FB_DEFAULT_PALETTE_LIGHTRED,
    CYG_FB_DEFAULT_PALETTE_LIGHTMAGENTA,
    CYG_FB_DEFAULT_PALETTE_YELLOW,
    CYG_FB_DEFAULT_PALETTE_WHITE
};

#define BLACK        colours[0x00]
#define BLUE         colours[0x01]
#define GREEN        colours[0x02]
#define CYAN         colours[0x03]
#define RED          colours[0x04]
#define MAGENTA      colours[0x05]
#define BROWN        colours[0x06]
#define LIGHTGREY    colours[0x07]
#define DARKGREY     colours[0x08]
#define LIGHTBLUE    colours[0x09]
#define LIGHTGREEN   colours[0x0A]
#define LIGHTCYAN    colours[0x0B]
#define LIGHTRED     colours[0x0C]
#define LIGHTMAGENTA colours[0x0D]
#define YELLOW       colours[0x0E]
#define WHITE        colours[0x0F]

static void
reset_colours_to_true(void)
{
#if (CYG_FB_FLAGS0_TRUE_COLOUR & CYG_FB_FLAGS0(FRAMEBUF))
    int i;

    for (i = 0; i < 16; i++) {
        colours[i] = CYG_FB_MAKE_COLOUR(FRAMEBUF,
                                        cyg_fb_palette_vga[i + i + i], cyg_fb_palette_vga[i + i + i + 1],cyg_fb_palette_vga[i + i + i + 2]);
    }
#endif
}

static void
fill_screen(void)
{
    int             result;

#define DEPTH   CYG_FB_DEPTH(FRAMEBUF)
#define WIDTH   CYG_FB_WIDTH(FRAMEBUF)
#define HEIGHT  CYG_FB_HEIGHT(FRAMEBUF)

    diag_printf("Frame buffer %s\n", STRING(FRAMEBUF));
    diag_printf("Depth %d, width %d, height %d\n", DEPTH, WIDTH, HEIGHT);

    result = CYG_FB_ON(FRAMEBUF);

    if (CYG_FB_FLAGS0(FRAMEBUF) & CYG_FB_FLAGS0_TRUE_COLOUR) {
        reset_colours_to_true();
    }

    // A white background
    CYG_FB_FILL_BLOCK(FRAMEBUF, 0, 0, WIDTH, HEIGHT, WHITE);
}

#endif // #ifdef CYGPKG_DEVS_FRAMEBUF_CORTEXM_STM32F10XXX

//=============================================================================
// Access touch screen

#define STACK_SIZE 8000

static int test_stack[(STACK_SIZE/sizeof(int))];
static cyg_thread test_thread;
static cyg_handle_t main_thread;

static int TS_Read(short *px, short *py, int *pb, int pd_fd)
{
    /* read a data point */
    short data[4];
    int bytes_read;

    bytes_read = read(pd_fd, data, sizeof(data));

    if (bytes_read != sizeof(data)) {
       return 0;
    }

    *px = data[1];
    *py = data[2];

    *pb = (data[0] ? 0x01 : 0);

    if(! *pb )
      return 3;          /* only have button data */
    else
      return 2;          /* have full set of data */
}

void
ts_test(cyg_addrword_t data)
{
    int   pd_fd;
    short px = 0, py = 0;
    int   pb, loop = 5000;
    int   ipx = 0, ipy = 0;
    int   cnt = 0;

    CYG_TEST_INIT();

    CYG_TEST_INFO("Touch screen test");

    pd_fd = open("/dev/ts", O_RDONLY | O_NONBLOCK);

    if (pd_fd < 0) {
       diag_printf("Error opening touch panel\n");
       return;
    }

    fill_screen();

    while(loop--){
      // Read all and average
      while(TS_Read(&px, &py, &pb, pd_fd)){
         cnt++;
         ipx += px;
         ipy += py;
      }
      if(cnt){
         px = ipx / cnt;
         py = ipy / cnt;
      }
      // Draw square from the pencil position
      if(pd_fd)
        CYG_FB_FILL_BLOCK(FRAMEBUF, px, py, 4, 4, RED);
      cyg_thread_delay(2);
      cnt = 0; ipx = 0; ipy = 0;
    }

    CYG_TEST_PASS_FINISH("Touch screen test");
}

//=============================================================================

void cyg_user_start(void)
{
    cyg_thread_create(2,                // Priority
                      ts_test,
                      0,
                      "Touch screen test",    // Name
                      test_stack,       // Stack
                      STACK_SIZE,       // Size
                      &main_thread,     // Handle
                      &test_thread // Thread data structure
        );
    cyg_thread_resume( main_thread);
}

//=============================================================================
// Print a message if we cannot run

#else // NA_MSG

void cyg_user_start(void)
{
    CYG_TEST_NA(NA_MSG);
}

#endif // NA_MSG

//=============================================================================
/* EOF stm32f10xxx_ts_test.c */
