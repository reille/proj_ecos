//==========================================================================
//
//      stm32fb.c
//
//      Provide framebuffer device driver for the stm32f10xxx target.
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
// Date:          2013-04-28
//
//###DESCRIPTIONEND####
//========================================================================

#include <cyg/io/framebuf.h>
#include <errno.h>
#include <string.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_io.h>

#include <cyg/io/stm32fb_lcd.inl>
#include <cyg/io/stm32fb.inl>


// Declare LCD 0
cyg_stm32_lcd_t cyg_stm32_lcd_0;


#ifdef CYGPKG_DEVS_FRAMEBUF_STM32_FB0
#define LCD_WIDTH		CYG_FB_fb0_WIDTH
#define LCD_HEIGHT		CYG_FB_fb0_HEIGHT
#else
#define LCD_WIDTH		400
#define LCD_HEIGHT		240
#endif


//=============================================================================
// LCD PIN mapping

#define LCD_CS_PIN     CYGHWR_HAL_STM32_GPIO(G, 12, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_RS_PIN     CYGHWR_HAL_STM32_GPIO(F,  0, OUT_50MHZ, ALT_PUSHPULL)

#define LCD_OE_PIN     CYGHWR_HAL_STM32_GPIO(D,  4, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_WE_PIN     CYGHWR_HAL_STM32_GPIO(D,  5, OUT_50MHZ, ALT_PUSHPULL)

#define LCD_A19_REV1_0 CYGHWR_HAL_STM32_GPIO(E,  3, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_A20_REV1_0 CYGHWR_HAL_STM32_GPIO(E,  4, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_A19_REV2_0 CYGHWR_HAL_STM32_GPIO(E,  5, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_A20_REV2_0 CYGHWR_HAL_STM32_GPIO(E,  6, OUT_50MHZ, ALT_PUSHPULL)

#define LCD_D0_PIN     CYGHWR_HAL_STM32_GPIO(D, 14, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D1_PIN     CYGHWR_HAL_STM32_GPIO(D, 15, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D2_PIN     CYGHWR_HAL_STM32_GPIO(D,  0, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D3_PIN     CYGHWR_HAL_STM32_GPIO(D,  1, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D4_PIN     CYGHWR_HAL_STM32_GPIO(E,  7, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D5_PIN     CYGHWR_HAL_STM32_GPIO(E,  8, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D6_PIN     CYGHWR_HAL_STM32_GPIO(E,  9, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D7_PIN     CYGHWR_HAL_STM32_GPIO(E, 10, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D8_PIN     CYGHWR_HAL_STM32_GPIO(E, 11, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D9_PIN     CYGHWR_HAL_STM32_GPIO(E, 12, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D10_PIN    CYGHWR_HAL_STM32_GPIO(E, 13, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D11_PIN    CYGHWR_HAL_STM32_GPIO(E, 14, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D12_PIN    CYGHWR_HAL_STM32_GPIO(E, 15, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D13_PIN    CYGHWR_HAL_STM32_GPIO(D,  8, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D14_PIN    CYGHWR_HAL_STM32_GPIO(D,  9, OUT_50MHZ, ALT_PUSHPULL)
#define LCD_D15_PIN    CYGHWR_HAL_STM32_GPIO(D, 10, OUT_50MHZ, ALT_PUSHPULL)


#define LCD_PWM_PIN    CYGHWR_HAL_STM32_GPIO(B,  1, OUT_50MHZ, OUT_PUSHPULL)


//=============================================================================
// LCD initialization

static void stm32_fb_lcd_clear_screen(cyg_uint16 colour)
{
    int i;

    lcd_set_cursor(0, 0);
    lcd_write_ir(0x0202);
	for (i = 0; i < (LCD_WIDTH * LCD_HEIGHT); i++) {
        lcd_write_ram(colour);
	}
}

static
void stm32_fb_lcd_hw_init(void)
{
    CYGHWR_HAL_STM32_GPIO_SET( LCD_OE_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_WE_PIN );

    CYGHWR_HAL_STM32_GPIO_SET( LCD_D0_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D1_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D2_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D3_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D4_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D5_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D6_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D7_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D8_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D9_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D10_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D11_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D12_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D13_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D14_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_D15_PIN );

    CYGHWR_HAL_STM32_GPIO_SET( LCD_CS_PIN );
    CYGHWR_HAL_STM32_GPIO_SET( LCD_RS_PIN );

    CYGHWR_HAL_STM32_GPIO_SET( LCD_A19_REV1_0);
    CYGHWR_HAL_STM32_GPIO_SET( LCD_A19_REV2_0);
    CYGHWR_HAL_STM32_GPIO_SET( LCD_A20_REV1_0);
    CYGHWR_HAL_STM32_GPIO_SET( LCD_A20_REV2_0);

    CYGHWR_HAL_STM32_GPIO_SET( LCD_PWM_PIN);
}

static void stm32_fb_lcd_5420_4001_init(struct cyg_fb * fb)
{
	/* 初始化LCD，写LCD寄存器进行配置 */
	lcd_write_reg(0x0000, 0x0000);
	lcd_write_reg(0x0001, 0x0100);
	lcd_write_reg(0x0002, 0x0100);

	/*
		R003H 寄存器很关键， Entry Mode ，决定了扫描方向
		参见：SPFD5420A.pdf 第15页

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

		虚拟坐标(x,y) 和物理坐标的转换关系
		x = 399 - py;
		y = px;

		py = 399 - x;
		px = y;

	*/
	lcd_write_reg(0x0003, 0x1018); /* 0x1018 1030 */

	lcd_write_reg(0x0008, 0x0808);
	lcd_write_reg(0x0009, 0x0001);
	lcd_write_reg(0x000B, 0x0010);
	lcd_write_reg(0x000C, 0x0000);
	lcd_write_reg(0x000F, 0x0000);
	lcd_write_reg(0x0007, 0x0001);
	lcd_write_reg(0x0010, 0x0013);
	lcd_write_reg(0x0011, 0x0501);
	lcd_write_reg(0x0012, 0x0300);
	lcd_write_reg(0x0020, 0x021E);
	lcd_write_reg(0x0021, 0x0202);
	lcd_write_reg(0x0090, 0x8000);
	lcd_write_reg(0x0100, 0x17B0);
	lcd_write_reg(0x0101, 0x0147);
	lcd_write_reg(0x0102, 0x0135);
	lcd_write_reg(0x0103, 0x0700);
	lcd_write_reg(0x0107, 0x0000);
	lcd_write_reg(0x0110, 0x0001);
	lcd_write_reg(0x0210, 0x0000);
	lcd_write_reg(0x0211, 0x00EF);
	lcd_write_reg(0x0212, 0x0000);
	lcd_write_reg(0x0213, 0x018F);
	lcd_write_reg(0x0280, 0x0000);
	lcd_write_reg(0x0281, 0x0004);
	lcd_write_reg(0x0282, 0x0000);
	lcd_write_reg(0x0300, 0x0101);
	lcd_write_reg(0x0301, 0x0B2C);
	lcd_write_reg(0x0302, 0x1030);
	lcd_write_reg(0x0303, 0x3010);
	lcd_write_reg(0x0304, 0x2C0B);
	lcd_write_reg(0x0305, 0x0101);
	lcd_write_reg(0x0306, 0x0807);
	lcd_write_reg(0x0307, 0x0708);
	lcd_write_reg(0x0308, 0x0107);
	lcd_write_reg(0x0309, 0x0105);
	lcd_write_reg(0x030A, 0x0F04);
	lcd_write_reg(0x030B, 0x0F00);
	lcd_write_reg(0x030C, 0x000F);
	lcd_write_reg(0x030D, 0x040F);
	lcd_write_reg(0x030E, 0x0300);
	lcd_write_reg(0x030F, 0x0701);
	lcd_write_reg(0x0400, 0x3500);
	lcd_write_reg(0x0401, 0x0001);
	lcd_write_reg(0x0404, 0x0000);
	lcd_write_reg(0x0500, 0x0000);
	lcd_write_reg(0x0501, 0x0000);
	lcd_write_reg(0x0502, 0x0000);
	lcd_write_reg(0x0503, 0x0000);
	lcd_write_reg(0x0504, 0x0000);
	lcd_write_reg(0x0505, 0x0000);
	lcd_write_reg(0x0600, 0x0000);
	lcd_write_reg(0x0606, 0x0000);
	lcd_write_reg(0x06F0, 0x0000);
	lcd_write_reg(0x07F0, 0x5420);
	lcd_write_reg(0x07DE, 0x0000);
	lcd_write_reg(0x07F2, 0x00DF);
	lcd_write_reg(0x07F3, 0x0810);
	lcd_write_reg(0x07F4, 0x0077);
	lcd_write_reg(0x07F5, 0x0021);
	lcd_write_reg(0x07F0, 0x0000);
	lcd_write_reg(0x0007, 0x0173);
#if 0
	/* 设置显示窗口 WINDOWS */
	lcd_write_reg(0x0210, 0);	/* 水平起始地址 */
	lcd_write_reg(0x0211, 239);	/* 水平结束坐标 */
	lcd_write_reg(0x0212, 0);	/* 垂直起始地址 */
	lcd_write_reg(0x0213, 399);	/* 垂直结束地址 */
#else
    CYG_FB_DEFAULT_WINDOW_AREA( ((cyg_stm32_lcd_t *)fb->fb_base) );
#endif
    /* 清除显存 */
    stm32_fb_lcd_clear_screen(0x0000);/* 黑色 */
}

static
void stm32_fb_lcd_init(struct cyg_fb * fb)
{
    cyg_uint16 lcd_id;

    // Set up FSMC NOR/SRAM bank 4 for LCD
    CYG_ADDRESS base = CYGHWR_HAL_STM32_FSMC;
    HAL_WRITE_UINT32( base+CYGHWR_HAL_STM32_FSMC_BCR4, 0x00001059 );
//  HAL_WRITE_UINT32( base+CYGHWR_HAL_STM32_FSMC_BTR4, 0x10000201 );
    HAL_WRITE_UINT32( base+CYGHWR_HAL_STM32_FSMC_BTR4, 0x10000502 );

    stm32_fb_lcd_hw_init();
    hal_delay_us(20 * 1000);

    lcd_id = lcd_read_id();
    diag_printf("LCD ID : %d (0x%04X)\n", lcd_id, lcd_id);
    if (lcd_id == 0x5420) {
        stm32_fb_lcd_5420_4001_init(fb);
    }
}

// Create a framebuffer device. This gets called from a C++ static constructor.
void _cyg_stm32_fb_instantiate(struct cyg_fb * fb)
{
    cyg_stm32_lcd_t * _lcd = (cyg_stm32_lcd_t *)fb->fb_base;

    if( _lcd == (cyg_uint32) CYG_PLF_FB_fb0_BASE){
        // Populate the LCD data
        _lcd->base   = 0;
        _lcd->width  = fb->fb_width;
        _lcd->height = fb->fb_height;
        _lcd->pwm_io = LCD_PWM_PIN;

        stm32_fb_lcd_init(fb);
    }
}


static
int stm32_fb_on(struct cyg_fb * fb)
{
    lcd_write_reg(7, 0x0173); /* 设置262K颜色并且打开显示 */
    return  ENOERR;
}

static
int stm32_fb_off(struct cyg_fb * fb)
{
    lcd_write_reg(7, 0x0);	/* 关闭显示 */
    return  ENOERR;
}

static
int stm32_fb_ioctl(struct cyg_fb* fb, cyg_ucount16 key, void* data, size_t* len)
{
    int result  = ENOERR;

    switch(key) {
    case CYG_FB_IOCTL_BACKLIGHT_SET:
        {
            cyg_fb_ioctl_backlight * backlight = (cyg_fb_ioctl_backlight *)data;
            if( backlight->fbbl_current == 0) {
                CYGHWR_HAL_STM32_GPIO_OUT (LCD_PWM_PIN, 0);
            } else {
                CYGHWR_HAL_STM32_GPIO_OUT (LCD_PWM_PIN, 1);
            }
        }
        break;
    default:
        result = ENOERR;
        break;
    }
    return  result;
}


// ----------------------------------------------------------------------------
// Landscape

void
stm32_fb_write_pixel_landscape_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_fb_colour colour)
{
    stm32_fb_write_pixel_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y, colour);
}

cyg_fb_colour
stm32_fb_read_pixel_landscape_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y)
{
    return stm32_fb_read_pixel_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y);
}

void
stm32_fb_write_hline_landscape_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 len, cyg_fb_colour colour)
{
    stm32_fb_write_hline_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y, len, colour);
}

void
stm32_fb_write_vline_landscape_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 len, cyg_fb_colour colour)
{
    stm32_fb_write_vline_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y, len, colour);
}

void
stm32_fb_fill_block_landscape_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height, cyg_fb_colour colour)
{
    stm32_fb_fill_block_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, colour);
}

void
stm32_fb_write_block_landscape_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height,
                             const void* source, cyg_ucount16 offset, cyg_ucount16 source_stride)
{
    stm32_fb_write_block_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, source, offset, source_stride);
}

void
stm32_fb_read_block_landscape_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height,
                            void* dest, cyg_ucount16 offset, cyg_ucount16 dest_stride)
{
    stm32_fb_read_block_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, dest, offset, dest_stride);
}

void
stm32_fb_move_block_landscape_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height,
                            cyg_ucount16 new_x, cyg_ucount16 new_y)
{
    stm32_fb_move_block_landscape_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, new_x, new_y);
}
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Portrait

void
stm32_fb_write_pixel_portrait_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_fb_colour colour)
{
    stm32_fb_write_pixel_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y, colour);
}

cyg_fb_colour
stm32_fb_read_pixel_portrait_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y)
{
    return stm32_fb_read_pixel_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y);
}

void
stm32_fb_write_hline_portrait_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 len, cyg_fb_colour colour)
{
    stm32_fb_write_hline_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y, len, colour);
}

void
stm32_fb_write_vline_portrait_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 len, cyg_fb_colour colour)
{
    stm32_fb_write_vline_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y, len, colour);
}

void
stm32_fb_fill_block_portrait_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height, cyg_fb_colour colour)
{
    stm32_fb_fill_block_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, colour);
}

void
stm32_fb_write_block_portrait_16(cyg_fb* fb,
                             cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height,
                             const void* source, cyg_ucount16 offset, cyg_ucount16 source_stride)
{
    stm32_fb_write_block_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, source, offset, source_stride);
}

void
stm32_fb_read_block_portrait_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height,
                            void* dest, cyg_ucount16 offset, cyg_ucount16 dest_stride)
{
    stm32_fb_read_block_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, dest, offset, dest_stride);
}

void
stm32_fb_move_block_portrait_16(cyg_fb* fb,
                            cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height,
                            cyg_ucount16 new_x, cyg_ucount16 new_y)
{
    stm32_fb_move_block_portrait_16_inl(fb->fb_base, fb->fb_stride, x, y, width, height, new_x, new_y);
}
// ----------------------------------------------------------------------------



#define FB_STM32_MAKE_FN1(_fn_, _orientation_, _suffix_)  \
                    stm32_fb_ ## _fn_ ## _ ## _orientation_ ## _ ##_suffix_
#define FB_STM32_MAKE_FN( _fn_, _orientation_, _suffix_)  \
                    FB_STM32_MAKE_FN1(_fn_, _orientation_, _suffix_)

#define FB_STM32_COLOUR_FN1(_fn_, _suffix_)  cyg_fb_dev_ ## _fn_ ## _ ## _suffix_
#define FB_STM32_COLOUR_FN( _fn_, _suffix_)  FB_STM32_COLOUR_FN1(_fn_, _suffix_)


#ifdef CYGPKG_DEVS_FRAMEBUF_STM32_FB0


CYG_FB_FRAMEBUFFER(CYG_FB_fb0_STRUCT,
                    CYG_FB_fb0_DEPTH,
                    CYG_FB_fb0_FORMAT,
                    CYG_FB_fb0_WIDTH,
                    CYG_FB_fb0_HEIGHT,
                    CYG_FB_fb0_VIEWPORT_WIDTH,
                    CYG_FB_fb0_VIEWPORT_HEIGHT,
                    CYG_FB_fb0_BASE,
                    CYG_FB_fb0_STRIDE,
                    CYG_FB_fb0_FLAGS0,
                    CYG_FB_fb0_FLAGS1,
                    CYG_FB_fb0_FLAGS2,
                    CYG_FB_fb0_FLAGS3,
                    (CYG_ADDRWORD) 0,
                    (CYG_ADDRWORD) 0,
                    (CYG_ADDRWORD) 0,
                    (CYG_ADDRWORD) 0,
                    &stm32_fb_on,
                    &stm32_fb_off,
                    &stm32_fb_ioctl,
                    &cyg_fb_nop_synch,
                    &cyg_fb_nop_read_palette,
                    &cyg_fb_nop_write_palette,
        &FB_STM32_COLOUR_FN(make_colour, 16bpp_true_565),
        &FB_STM32_COLOUR_FN(break_colour, 16bpp_true_565),
        FB_STM32_MAKE_FN(write_pixel, CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
        FB_STM32_MAKE_FN(read_pixel,  CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
        FB_STM32_MAKE_FN(write_hline, CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
        FB_STM32_MAKE_FN(write_vline, CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
        FB_STM32_MAKE_FN(fill_block,  CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
        FB_STM32_MAKE_FN(write_block, CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
        FB_STM32_MAKE_FN(read_block,  CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
        FB_STM32_MAKE_FN(move_block,  CYG_FB_fb0_ORIENTATION, CYG_FB_fb0_DEPTH),
                    0, 0, 0, 0
    );
#endif


//-------------------------------------------------------------------------
// End of file
