//==========================================================================
//
//      stm32fb_lcd.inl
//
//      STM32 LCD controller platform specific code.
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
//###DESCRIPTIONBEGIN####
//
// Author(s):     reille
// Date:          2013-04-30
//
//###DESCRIPTIONEND####
//========================================================================

#ifndef CYGONCE_DEVS_FRAMEBUF_STM32FB_LCD_INL
#define CYGONCE_DEVS_FRAMEBUF_STM32FB_LCD_INL

#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_if.h>


/*
 * \breif
 * Define the LCD base address: index register and data register address.
 */
#define LCD_BASE        (0x60000000 | 0x0C000000)
#define LCD_REG			(LCD_BASE)
#define LCD_RAM			(LCD_BASE + 2)

/*
 * \breif
 * Define LCD register index (register address).
 */
#define LCDR_GRAM_X		0x200
#define LCDR_GRAM_Y		0x201
#define LCDR_GRAM		0x202


// Internal structure for the platform dependent operations
typedef struct cyg_stm32_lcd_t
{
    cyg_uint32   base;     /* base address, N/A */
    cyg_uint32   width;    /* width */
    cyg_uint32   height;   /* height */
    cyg_uint32   pwm_io;   /* IO controlling the LCD backlight */
} cyg_stm32_lcd_t;

// Declare the LCD 0 screen
externC cyg_stm32_lcd_t cyg_stm32_lcd_0;

#define CYG_PLF_FB_fb0_BASE        &cyg_stm32_lcd_0


//=============================================================================
// LCD register operation functions

static __inline__ void lcd_write_reg(CYG_WORD16 _reg, cyg_uint16 _val)
{
    HAL_WRITE_UINT16(LCD_REG, _reg);/* Write 16-bit index, then write register */
    HAL_WRITE_UINT16(LCD_RAM, _val);/* Write 16-bit register value */
}

static __inline__ cyg_uint16 lcd_read_reg(CYG_WORD16 _reg)
{
    cyg_uint16 val;
    HAL_WRITE_UINT16(LCD_REG, _reg);/* Write 16-bit Index, then read register */
    HAL_READ_UINT16(LCD_RAM, val);  /* Read value from 16-bit register */
    return  val;
}

// Write to Index Register (IR)
static __inline__ void lcd_write_ir(CYG_WORD16 _reg)
{
    HAL_WRITE_UINT16(LCD_REG, _reg);
}

// Write to GRAM
static __inline__ void lcd_write_ram(cyg_uint16 _val)
{
    HAL_WRITE_UINT16(LCD_RAM, _val);
}

// Read from GRAM
static __inline__ cyg_uint16 lcd_read_ram(void)
{
    cyg_uint16 val;
    HAL_READ_UINT16(LCD_RAM, val);
    return (val);
}

// Read LCD ID
static __inline__ cyg_uint16 lcd_read_id(void)
{
    return  lcd_read_reg(0x0000);
}

static __inline__ void lcd_set_cursor(cyg_uint16 x, cyg_uint16 y)
{
	/*
		px，py 是物理坐标， x，y是虚拟坐标
		转换公式:
		py = 399 - x;
		px = y;
	*/

    /* Set cursor */
	lcd_write_reg(0x0200, y);  	    /* px */
	lcd_write_reg(0x0201, 399 - x);	/* py */

//	lcd_write_reg(0x0200, x);  	    /* px */
//	lcd_write_reg(0x0201, y);	/* py */
}

static __inline__ void lcd_put_pixel(cyg_uint16 x, cyg_uint16 y, cyg_uint16 colour)
{
    lcd_set_cursor(x, y);
    lcd_write_ir(LCDR_GRAM);
    lcd_write_ram(colour);
}

static __inline__ cyg_uint16 lcd_get_pixel(cyg_uint16 x, cyg_uint16 y)
{
    cyg_uint16  _pixel_;

    lcd_set_cursor(x, y);
    lcd_write_ir(LCDR_GRAM);
    _pixel_ = lcd_read_ram();
    return  _pixel_;
}

static __inline__ void lcd_set_disp_win(cyg_stm32_lcd_t * lcd,
                    cyg_uint16 x, cyg_uint16 y, cyg_uint16 w, cyg_uint16 h)
{
	cyg_uint16 px, py;

	/*
		240x400屏幕物理坐标(px,py)如下:
		    R003 = 0x1018                  R003 = 0x1008
		  -------------------          -------------------
		 |(0,0)              |        |(0,0)              |
		 |                   |        |					  |
		 |  ^           ^    |        |   ^           ^   |
		 |  |           |    |        |   |           |   |
		 |  |           |    |        |   |           |   |
		 |  |           |    |        |   |           |   |
		 |  |  ------>  |    |        |   | <------   |   |
		 |  |           |    |        |   |           |   |
		 |  |           |    |        |   |           |   |
		 |  |           |    |        |   |           |   |
		 |  |           |    |        |   |           |   |
		 |                   |        |					  |
		 |                   |        |                   |
		 |      (x=239,y=399)|        |      (x=239,y=399)|
		 |-------------------|        |-------------------|
		 |                   |        |                   |
		  -------------------          -------------------

		按照安富莱开发板LCD的方向，我们期望的虚拟坐标和扫描方向如下：(和上图第1个吻合)
		 --------------------------------
		|  |(0,0)                        |
		|  |     --------->              |
		|  |         |                   |
		|  |         |                   |
		|  |         |                   |
		|  |         V                   |
		|  |     --------->              |
		|  |                    (399,239)|
		 --------------------------------
	虚拟坐标和物理坐标转换关系：
		x = 399 - py;
		y = px;

		py = 399 - x;
		px = y;
	*/
#if 0

	py = 399 - _usX;
	px = _usY;

	/* 设置显示窗口 WINDOWS */
	LCD_WriteReg(0x0210, px);						/* 水平起始地址 */
	LCD_WriteReg(0x0211, px + (_usHeight - 1));		/* 水平结束坐标 */
	LCD_WriteReg(0x0212, py + 1 - _usWidth);		/* 垂直起始地址 */
	LCD_WriteReg(0x0213, py);						/* 垂直结束地址 */

	LCD_SetCursor(_usX, _usY);
#endif
    py = (lcd->width - 1) - x;
    px = y;

	lcd_write_reg(0x0210, px);						/* 水平起始地址 */
	lcd_write_reg(0x0211, px + (h - 1));	        /* 水平结束坐标 */
	lcd_write_reg(0x0212, py + 1 - w);		        /* 垂直起始地址 */
	lcd_write_reg(0x0213, py);						/* 垂直结束地址 */

    lcd_set_cursor(x, y);
}

#endif/* CYGONCE_DEVS_FRAMEBUF_STM32FB_LCD_INL */

//-------------------------------------------------------------------------
// End of file
