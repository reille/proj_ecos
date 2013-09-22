//==========================================================================
//
//      tsc2046.c
//
//      Touch screen drivers for the TSC2046
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
// Author(s):    gthomas Touchscreen driver for Agilent AAED2000
// Contributors: ccoutand, updated for TSC2046 over SPI interface
// Date:         2012-01-05
// Purpose:
// Description:  Touch screen drivers for the TSC2046
//
//####DESCRIPTIONEND####
//
//==========================================================================


#define DEBUG   0

#include <pkgconf/devs_touch_spi_tsc2046.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/drv_api.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/io/spi.h>

#include <cyg/fileio/fileio.h>  // For select() functionality

static cyg_selinfo      ts_select_info;
static cyg_bool         ts_select_active;

#include <cyg/io/devtab.h>


// Self calibrate enable
#define TSC_SELF_CALIBARATE_EN  0

/* TSC2046 flags */
#define TSC_START           (1 << 7)
#define TSC_MEASURE_X       (0x01 << 4)
#define TSC_MEASURE_Y       (0x05 << 4)
#define TSC_MODE_12_BIT      0x00
#define TSC_SER              0x04
#define TSC_DFR              0x00
#define TSC_PD0              0x00

// Misc constants
#define X_THRESHOLD         0x80
#define Y_THRESHOLD         0x80

// Functions in this module

static Cyg_ErrNo ts_read(cyg_io_handle_t handle,
                           void *buffer,
                           cyg_uint32 *len);
static cyg_bool  ts_select(cyg_io_handle_t handle,
                           cyg_uint32 which,
                           cyg_addrword_t info);
static Cyg_ErrNo ts_set_config(cyg_io_handle_t handle,
                           cyg_uint32 key,
                           const void *buffer,
                           cyg_uint32 *len);
static Cyg_ErrNo ts_get_config(cyg_io_handle_t handle,
                           cyg_uint32 key,
                           void *buffer,
                           cyg_uint32 *len);
static bool      ts_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo ts_lookup(struct cyg_devtab_entry **tab,
                           struct cyg_devtab_entry *st,
                           const char *name);

#include CYGDAT_DEVS_TOUCH_SPI_TSC2046_INL // Instantiate Device

struct _event {
    short button_state;
    short xPos, yPos;
    short _unused;
};
#define MAX_EVENTS CYGNUM_DEVS_TOUCH_SPI_TSC2046_EVENT_BUFFER_SIZE
static int    num_events;
static int    _event_put, _event_get;
static struct _event _events[MAX_EVENTS];

static bool _is_open = false;

#define STACK_SIZE CYGNUM_HAL_STACK_SIZE_TYPICAL
static char ts_scan_stack[STACK_SIZE];
static cyg_thread ts_scan_thread_data;
static cyg_handle_t ts_scan_thread_handle;

#define SCAN_DELAY ((1000/CYGNUM_DEVS_TOUCH_SPI_TSC2046_SCAN_FREQUENCY)/10)

typedef struct {
    short min;
    short max;
    short span;
} bounds;

static bounds xBounds = {1024, 0, 1024};
static bounds yBounds = {1024, 0, 1024};

static Cyg_ErrNo
ts_read(cyg_io_handle_t handle,
        void *buffer,
        cyg_uint32 *len)
{
    struct _event *ev;
    int tot = *len;
    unsigned char *bp = (unsigned char *)buffer;

    cyg_scheduler_lock();  // Prevent interaction with DSR code
    while (tot >= sizeof(struct _event)) {
        if (num_events > 0) {
            ev = &_events[_event_get++];
            if (_event_get == MAX_EVENTS) {
                _event_get = 0;
            }

#if TSC_SELF_CALIBARATE_EN
            // Self calibrate
            if (ev->xPos > xBounds.max) xBounds.max = ev->xPos;
            if (ev->xPos < xBounds.min) xBounds.min = ev->xPos;
            if (ev->yPos > yBounds.max) yBounds.max = ev->yPos;
            if (ev->yPos < yBounds.min) yBounds.min = ev->yPos;
            if ((xBounds.span = xBounds.max - xBounds.min) <= 1) {
                xBounds.span = 1;
            }
            if ((yBounds.span = yBounds.max - yBounds.min) <= 1) {
                yBounds.span = 1;
            }
            // Scale values - done here so these potentially lengthy
            // operations take place outside of interrupt processing
#ifdef CYGDAT_DEVS_TOUCH_SPI_TSC2046_TRACE
            diag_printf("Raw[%d,%d], X[%d,%d,%d], Y[%d,%d,%d]",
                        ev->xPos, ev->yPos,
                        xBounds.max, xBounds.min, xBounds.span,
                        yBounds.max, yBounds.min, yBounds.span);
#endif

#ifdef CYGSEM_DEVS_TOUCH_SPI_TSC2046_HORIZONTAL_SWAP
            ev->xPos = CYGNUM_DEVS_TOUCH_SPI_TSC2046_HORIZONTAL_SPAN -
                           (((xBounds.max - ev->xPos) * CYGNUM_DEVS_TOUCH_SPI_TSC2046_HORIZONTAL_SPAN) / xBounds.span);
#else
            ev->xPos = (((xBounds.max - ev->xPos) * CYGNUM_DEVS_TOUCH_SPI_TSC2046_HORIZONTAL_SPAN) / xBounds.span);
#endif

#ifdef CYGSEM_DEVS_TOUCH_SPI_TSC2046_VERTICAL_SWAP
            ev->yPos = CYGNUM_DEVS_TOUCH_SPI_TSC2046_VERTICAL_SPAN -
                           (((yBounds.max - ev->yPos) * CYGNUM_DEVS_TOUCH_SPI_TSC2046_VERTICAL_SPAN) / yBounds.span);
#else
            ev->yPos = (((yBounds.max - ev->yPos) * CYGNUM_DEVS_TOUCH_SPI_TSC2046_VERTICAL_SPAN) / yBounds.span);
#endif

#ifdef CYGDAT_DEVS_TOUCH_SPI_TSC2046_TRACE
            diag_printf(", Cooked[%d,%d]\n",
                        ev->xPos, ev->yPos);
#endif

#endif /* TSC_SELF_CALIBARATE_EN */

            memcpy(bp, ev, sizeof(*ev));
            bp += sizeof(*ev);
            tot -= sizeof(*ev);
            num_events--;
        } else {
            break;  // No more events
        }
    }
    cyg_scheduler_unlock(); // Allow DSRs again
    *len -= tot;
    return ENOERR;
}

static cyg_bool
ts_select(cyg_io_handle_t handle,
          cyg_uint32 which,
          cyg_addrword_t info)
{
    if (which == CYG_FREAD) {
        cyg_scheduler_lock();  // Prevent interaction with DSR code
        if (num_events > 0) {
            cyg_scheduler_unlock();  // Reallow interaction with DSR code
            return true;
        }
        if (!ts_select_active) {
            ts_select_active = true;
            cyg_selrecord(info, &ts_select_info);
        }
        cyg_scheduler_unlock();  // Reallow interaction with DSR code
    }
    return false;
}

static Cyg_ErrNo
ts_set_config(cyg_io_handle_t handle,
              cyg_uint32 key,
              const void *buffer,
              cyg_uint32 *len)
{
    return EINVAL;
}

static Cyg_ErrNo
ts_get_config(cyg_io_handle_t handle,
              cyg_uint32 key,
              void *buffer,
              cyg_uint32 *len)
{
    return EINVAL;
}

static bool
ts_init(struct cyg_devtab_entry *tab)
{
    cyg_selinit(&ts_select_info);
    return true;
}

static short
read_ts(cyg_uint8 axis)
{
#if 1
    cyg_spi_device *spi_device = (cyg_spi_device *) ts_dev.priv;

    // Implement the read operation as a multistage SPI transaction.
    cyg_spi_transaction_begin(spi_device);

    cyg_uint8 tx[8] = {0};
    cyg_uint8 rx[8] = {0};
    tx[0] = (axis | TSC_START | TSC_MODE_12_BIT | TSC_PD0 | TSC_DFR);
    cyg_spi_transaction_transfer(spi_device, true, 8, tx, rx, false);

    cyg_spi_transaction_end(spi_device);

    /* 以下是按照安富莱开发板附带例程的处理方式 */
    /* 读ADC结果, 12位ADC值的高位先传，前12bit有效，最后4bit填0 */
    cyg_uint16 r = 0;
    r = rx[1];
    r <<= 8;
    r += rx[2];
    r >>= 3;/* 注意这里的处理 *//* 右移3位，保留12位有效数字 */

#if DEBUG > 1
    diag_printf("%s: %02X, %02X, %02X, %02X : %02X, %02X, %02X, %02X -- r%s = %d\n",
        (axis == TSC_MEASURE_X ? "MEASURE_X" : "MEASURE_Y"),
        rx[0],rx[1],rx[2],rx[3],rx[4],rx[5],rx[6],rx[7],
        (axis == TSC_MEASURE_X ? "x" : "y"), r);
#endif



    return  (short)r;

#else
	short res;

    cyg_spi_device *spi_device = (cyg_spi_device *) ts_dev.priv;
    const cyg_uint8 tx_buf[1] = { axis | TSC_START | TSC_MODE_12_BIT | TSC_PD0 | TSC_DFR};
    cyg_uint8 rx_buf[2];

    // Implement the read operation as a multistage SPI transaction.
    cyg_spi_transaction_begin(spi_device);
    cyg_spi_transaction_transfer(spi_device, true,
                                 1, tx_buf, NULL, false);
    cyg_spi_transaction_transfer(spi_device, true, 2, NULL,
                                 rx_buf, false);
    cyg_spi_transaction_end(spi_device);

    res = ( rx_buf[0] << 4 ) + (( rx_buf[1] & 0xF0 ) >> 4);

#if DEBUG > 1
    diag_printf("%s: rx_buf[0] = 0x%02X, rx_buf[1] = 0x%02X --- r%s = %d\n",
        (axis == TSC_MEASURE_X ? "MEASURE_X" : "MEASURE_Y"),
        rx_buf[0], rx_buf[1],
        (axis == TSC_MEASURE_X ? "x" : "y"), res);
#endif

    return res;
#endif
}

static void
ts_scan(cyg_addrword_t param)
{
    short lastX, lastY;
    short x, y;
    struct _event *ev;
    bool pen_down;

#if DEBUG > 0
    diag_printf("Touch Screen thread started\n");
#endif
    // Discard the first sample - it's always 0
    x = read_ts(TSC_MEASURE_X);
    y = read_ts(TSC_MEASURE_Y);
    lastX = lastY = -1;
    pen_down = false;
    while (true) {
        cyg_thread_delay(SCAN_DELAY);
        if ((ts_dev.get_pen_state() == 0)) {
            // Pen is down
            x = read_ts(TSC_MEASURE_X);
            y = read_ts(TSC_MEASURE_Y);
#if DEBUG > 0
            diag_printf("Pen is down, x = %d, y = %d, num_events = %d .......\n",
                            x, y, num_events);
#endif
            if ((x < X_THRESHOLD) || (y < Y_THRESHOLD)) {
                // Ignore 'bad' samples
                continue;
            }
            lastX = x;  lastY = y;
            pen_down = true;
        } else {
            if (pen_down) {
                // Capture first 'up' event
                pen_down = false;
                x = lastX;
                y = lastY;
            } else {
                continue;  // Nothing new to report
            }
        }
        if (num_events < MAX_EVENTS) {
            num_events++;
            ev = &_events[_event_put++];
            if (_event_put == MAX_EVENTS) {
                _event_put = 0;
            }
            ev->button_state = pen_down ? 0x04 : 0x00;
            ev->xPos = x;
            ev->yPos = y;
            if (ts_select_active) {
                ts_select_active = false;
                cyg_selwakeup(&ts_select_info);
            }
        }
    }
}

static Cyg_ErrNo
ts_lookup(struct cyg_devtab_entry **tab,
          struct cyg_devtab_entry *st,
          const char *name)
{
    if (!_is_open) {
        _is_open = true;
        cyg_thread_create(6,                      // Priority
                          ts_scan,                // entry
                          0,                      // entry parameter
                          "Touch Screen scan",    // Name
                          &ts_scan_stack[0],      // Stack
                          STACK_SIZE,             // Size
                          &ts_scan_thread_handle, // Handle
                          &ts_scan_thread_data    // Thread data structure
        );
        cyg_thread_resume(ts_scan_thread_handle);    // Start it
    }
    return ENOERR;
}
