//==========================================================================
//
//      stm32fb.inl
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
// Date:          2013-04-30
//
//###DESCRIPTIONEND####
//========================================================================

#ifndef CYGONCE_DEVS_FRAMEBUF_STM32FB_INL
#define CYGONCE_DEVS_FRAMEBUF_STM32FB_INL


// The always_inline attribute must be a applied to a declaration, not a
// definition, so combine the two via a single macro.
#define CYG_FB_DEVS_FRAMEBUF_INLINE_FN(_type_, _name_, _args_)                        \
static __inline__ _type_ _name_ _args_ __attribute__((__always_inline__));            \
static __inline__ _type_ _name_ _args_


/* 设置显示窗口 WINDOWS */
# define CYG_FB_DEFAULT_WINDOW_AREA( _fbaddr_ )                 \
    CYG_MACRO_START                                             \
    cyg_stm32_lcd_t * _lcd = (cyg_stm32_lcd_t *)_fbaddr_;       \
    if ( _lcd->width > _lcd->height ) {                         \
       lcd_set_disp_win(_lcd, 0, 0, _lcd->width, _lcd->height); \
    } else {                                                    \
       lcd_set_disp_win(_lcd, 0, 0, _lcd->height, _lcd->width); \
    }                                                           \
    CYG_MACRO_END



// ----------------------------------------------------------------------------
// LCD in landscape mode

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_pixel_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_fb_colour _colour_))
{
    lcd_put_pixel(_x_,_y_,_colour_);
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(cyg_fb_colour,
                         stm32_fb_read_pixel_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_))
{
    return  lcd_get_pixel(_x_,_y_);
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_hline_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _len_, cyg_fb_colour _colour_))
{
    cyg_uint32 pixels;

    lcd_set_cursor(_x_,_y_);
    /* 先横向扫描，再纵向扫描 */
    lcd_write_ir(LCDR_GRAM);
    for (pixels = 0; pixels < _len_; pixels++) {
        lcd_write_ram(_colour_);
    }
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_vline_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _len_, cyg_fb_colour _colour_))
{
    cyg_uint16 _len;

    lcd_set_disp_win((cyg_stm32_lcd_t *)_fbaddr_, _x_,_y_, 1,_len_);
    lcd_write_ir(LCDR_GRAM);
    for (_len  = _len_ ; _len; _len--) {
        lcd_write_ram(_colour_);
    }
    CYG_FB_DEFAULT_WINDOW_AREA( _fbaddr_ );
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_fill_block_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _width_, cyg_ucount16 _height_, cyg_fb_colour _colour_))
{
    cyg_uint16 _width, _height;

    lcd_set_disp_win((cyg_stm32_lcd_t *)_fbaddr_, _x_,_y_,_width_,_height_);
    lcd_write_ir(LCDR_GRAM);
    for (_height = _height_ ; _height; _height--) {
        for (_width  = _width_ ; _width; _width--) {
            lcd_write_ram(_colour_);
        }
    }

    CYG_FB_DEFAULT_WINDOW_AREA( _fbaddr_ );
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_block_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _width_, cyg_ucount16 _height_, const void* _source_, cyg_ucount16 _offset_, cyg_ucount16 _source_stride8_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_read_block_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _width_, cyg_ucount16 _height_, void* _dest_, cyg_ucount16 _offset_, cyg_ucount16 _dest_stride8_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_move_block_landscape_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height, cyg_ucount16 new_x, cyg_ucount16 new_y))
{
    // TODO
}

// ----------------------------------------------------------------------------
// LCD in portrait mode

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_pixel_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_fb_colour _colour_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(cyg_fb_colour,
                         stm32_fb_read_pixel_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_))
{
    cyg_uint16  _pixel_;
    // TODO
    return _pixel_;
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_hline_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _len_, cyg_fb_colour _colour_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_vline_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride_, cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _len_, cyg_fb_colour _colour_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_fill_block_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _width_, cyg_ucount16 _height_, cyg_fb_colour _colour_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_write_block_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _width_, cyg_ucount16 _height_, const void* _source_, cyg_ucount16 _offset_, cyg_ucount16 _source_stride8_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_read_block_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 _x_, cyg_ucount16 _y_, cyg_ucount16 _width_, cyg_ucount16 _height_, void* _dest_, cyg_ucount16 _offset_, cyg_ucount16 _dest_stride8_))
{
    // TODO
}

CYG_FB_DEVS_FRAMEBUF_INLINE_FN(void,
                         stm32_fb_move_block_portrait_16_inl,
                        (void* _fbaddr_, cyg_ucount16 _stride8_,
                         cyg_ucount16 x, cyg_ucount16 y, cyg_ucount16 width, cyg_ucount16 height, cyg_ucount16 new_x, cyg_ucount16 new_y))
{
    // TODO
}

// ----------------------------------------------------------------------------
#if 0
#define STM32_FB_PIXELx_VAR_16(_fb_, _which_)  cyg_uint16  _cyg_fb_xpixelpos16_ ## _fb_ ## _which_; cyg_uint16  _cyg_fb_ypixelpos16_ ## _fb_ ## _which_; cyg_ili9325_lcd_t * _cyg_fb_lcd_ ## _fb_ ## _which_ = NULL

#define STM32_FB_PIXELx_SET_16(_fb_, _which_, _fbaddr_, _stride8_, _x_, _y_)                                               \
    CYG_MACRO_START                                                                                                              \
    _cyg_fb_lcd_ ## _fb_ ## _which_ = (cyg_ili9325_lcd_t *)_fbaddr_;                                                             \
    _cyg_fb_xpixelpos16_ ## _fb_ ## _which_  = (_x_);                                                                            \
    _cyg_fb_ypixelpos16_ ## _fb_ ## _which_  = (_y_);                                                                            \
    CYG_MACRO_END

#define STM32_FB_PIXELx_GET_16(_fb_, _which_, _fbaddr_, _stride8_, _x_ ,_y_)                                               \
    CYG_MACRO_START                                                                                                              \
    _y_ = _cyg_fb_ypixelpos16_ ## _fb_ ## _which_;                                                                               \
    _x_ = _cyg_fb_xpixelpos16_ ## _fb_ ## _which_;                                                                               \
    CYG_MACRO_END

#define STM32_FB_PIXELx_ADDX_16(_fb_, _which_, _stride8_, _incr_)                                                          \
    CYG_MACRO_START                                                                                                              \
    _cyg_fb_xpixelpos16_ ## _fb_ ## _which_ += (_incr_);                                                                         \
    CYG_MACRO_END

#define STM32_FB_PIXELx_ADDY_16(_fb_, _which_, _stride8_, _incr_)                                                          \
    CYG_MACRO_START                                                                                                              \
    _cyg_fb_ypixelpos16_ ## _fb_ ## _which_ += (_incr_);                                                                         \
    CYG_MACRO_END

// ----------------------------------------------------------------------------
// LCD in landscape mode

#define STM32_FB_PIXELx_WRITE_LANDSCAPE_16(_fb_, _which_, _colour_)                                                        \
    CYG_MACRO_START                                                                                                              \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_VGAS, _cyg_fb_xpixelpos16_ ## _fb_ ## _which_ );   \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_HGAS, _cyg_fb_ypixelpos16_ ## _fb_ ## _which_ );   \
    HAL_PLF_LCD_WRITE_IR (_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_WDG);                                              \
    HAL_PLF_LCD_WRITE_RAM(_cyg_fb_lcd_ ## _fb_ ## _which_, _colour_);                                                            \
    CYG_MACRO_END

#define STM32_FB_PIXELx_READ_LANDSCAPE_16(_fb_, _which_)                                                                   \
    ({                                                                                                                           \
    cyg_uint16 _pixel;                                                                                                           \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_VGAS, _cyg_fb_xpixelpos16_ ## _fb_ ## _which_ );   \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_HGAS, _cyg_fb_ypixelpos16_ ## _fb_ ## _which_ );   \
    _pixel = HAL_PLF_LCD_READ_RAM(_lcd);                                                                                         \
    _pixel;                                                                                                                      \
    })

// ----------------------------------------------------------------------------
// LCD in portrait mode

#define STM32_FB_PIXELx_WRITE_PORTRAIT_16(_fb_, _which_, _colour_)                                                         \
    CYG_MACRO_START                                                                                                              \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_HGAS, _cyg_fb_xpixelpos16_ ## _fb_ ## _which_ );   \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_VGAS, _cyg_fb_ypixelpos16_ ## _fb_ ## _which_ );   \
    HAL_PLF_LCD_WRITE_IR (_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_WDG);                                              \
    HAL_PLF_LCD_WRITE_RAM(_cyg_fb_lcd_ ## _fb_ ## _which_, _colour_);                                                            \
    CYG_MACRO_END

#define STM32_FB_PIXELx_READ_PORTRAIT_16(_fb_, _which_)                                                                    \
    ({                                                                                                                           \
    cyg_uint16 _pixel;                                                                                                           \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_HGAS, _cyg_fb_xpixelpos16_ ## _fb_ ## _which_ );   \
    HAL_PLF_LCD_WRITE_REG(_cyg_fb_lcd_ ## _fb_ ## _which_, ILI9325_CONTROLLER_VGAS, _cyg_fb_ypixelpos16_ ## _fb_ ## _which_ );   \
    _pixel = HAL_PLF_LCD_READ_RAM(_lcd);                                                                                         \
    _pixel;                                                                                                                      \
    })

#define STM32_FB_PIXELx_FLUSHABS_16(_fb_, _which_, _x0_, _y0_, _x1_, _y1_)                                                 \
    CYG_MACRO_START                                                                                                              \
    CYG_MACRO_END

#define STM32_FB_PIXELx_FLUSHREL_16(_fb_, _which_, _x0_, _y0_, _dx_, _dy_)                                                 \
    CYG_MACRO_START                                                                                                              \
    CYG_MACRO_END
#endif


#endif/* CYGONCE_DEVS_FRAMEBUF_STM32FB_INL */

//-------------------------------------------------------------------------
// End of file
