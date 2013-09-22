//==========================================================================
//
//      i2c_stm32_gpio.c
//
//      GPIO-based bitbanging I2C driver for STM32
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 2010 Free Software Foundation, Inc.
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
// Contributors:
// Date:         2013-05-25
// Description:  GPIO-based bitbanging I2C driver for STM32
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>
#include <pkgconf/devs_i2c_cortexm_stm32_gpio.h>

#include <cyg/infra/cyg_type.h>
#include <cyg/infra/cyg_ass.h>
#include <cyg/infra/diag.h>
#include <cyg/io/i2c.h>
#include <cyg/io/i2c_stm32_gpio.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/drv_api.h>


// Enable for printing out debugging
#if 0
  #define DEBUG(_format_, ...) diag_printf(_format_, ## __VA_ARGS__)
#else
  #define DEBUG(_format_, ...)
#endif


#define I2C_SCL_OUT()   CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_PIN_OUT(B, 6, OPENDRAIN, NONE, AT_LEAST(50)))
#define I2C_SDA_OUT()   CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_PIN_OUT(B, 7, OPENDRAIN, NONE, AT_LEAST(50)))
#define I2C_SDA_INPUT() CYGHWR_HAL_STM32_GPIO_SET(CYGHWR_HAL_STM32_PIN_IN(B,  7, FLOATING))

#define I2C_SCL_L()     CYGHWR_HAL_STM32_GPIO_OUT(CYGHWR_HAL_STM32_PIN_OUT(B, 6, OPENDRAIN, NONE, AT_LEAST(50)), 0)
#define I2C_SCL_H()     CYGHWR_HAL_STM32_GPIO_OUT(CYGHWR_HAL_STM32_PIN_OUT(B, 6, OPENDRAIN, NONE, AT_LEAST(50)), 1)
#define I2C_SDA_L()     CYGHWR_HAL_STM32_GPIO_OUT(CYGHWR_HAL_STM32_PIN_OUT(B, 7, OPENDRAIN, NONE, AT_LEAST(50)), 0)
#define I2C_SDA_H()    CYGHWR_HAL_STM32_GPIO_OUT(CYGHWR_HAL_STM32_PIN_OUT(B,  7, OPENDRAIN, NONE, AT_LEAST(50)), 1)

#define I2C_SDA_R(val) CYGHWR_HAL_STM32_GPIO_IN(CYGHWR_HAL_STM32_PIN_IN(B, 7, FLOATING), (val))

#define I2C_DELAY()   hal_delay_us(2)


static cyg_drv_mutex_t i2c_lock;
static int init_done = 0;



// ----------------------------------------------------------------------------

static void i2c_gpio_start(void)
{
	/* 当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号 */
	I2C_SDA_H();
	I2C_SCL_H();
	I2C_DELAY();
	I2C_SDA_L();
	I2C_DELAY();
	I2C_SCL_L();
	I2C_DELAY();
}

static void i2c_gpio_stop(void)
{
	/* 当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号 */
	I2C_SDA_L();
	I2C_SCL_H();
	I2C_DELAY();
	I2C_SDA_H();
}

static void i2c_gpio_write8(cyg_uint8 data)
{
	cyg_uint8 i;

	/* 先发送字节的高位bit7 */
	for (i = 0; i < 8; i++)	{
		if (data & 0x80) {
			I2C_SDA_H();
		} else {
			I2C_SDA_L();
		}
		I2C_DELAY();
		I2C_SCL_H();
		I2C_DELAY();
		I2C_SCL_L();
		if (i == 7)	{
			 I2C_SDA_H(); /* 释放总线 */
		}
		data <<= 1;	/* 左移一个bit */
		I2C_DELAY();
	}
}

static cyg_uint8 i2c_gpio_read8(void)
{
	cyg_uint8 i;
	cyg_uint8 value;
    cyg_uint32 sda_val;

    I2C_SDA_INPUT();

	/* 读到第1个bit为数据的bit7 */
	value = 0;
	for (i = 0; i < 8; i++)	{
		value <<= 1;
		I2C_SCL_H();
		I2C_DELAY();

        I2C_SDA_R((&sda_val));

		if (sda_val)	{
			value++;
		}
		I2C_SCL_L();
		I2C_DELAY();
	}

    I2C_SDA_OUT();

	return value;
}

static cyg_uint8 i2c_gpio_wait_ack(void)
{
	cyg_uint8 re;
    cyg_uint32 sda_val;

    I2C_SDA_INPUT();

	I2C_SDA_H();	/* CPU释放SDA总线 */
	I2C_DELAY();
	I2C_SCL_H();	/* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
	I2C_DELAY();

    I2C_SDA_R((&sda_val));

	if (sda_val) {	/* CPU读取SDA口线状态 */
		re = 1;
	} else {
		re = 0;
	}
	I2C_SCL_L();
	I2C_DELAY();

    I2C_SDA_OUT();

	return re;
}

static void i2c_gpio_ack(void)
{
	I2C_SDA_L();	/* CPU驱动SDA = 0 */
	I2C_DELAY();
	I2C_SCL_H();	/* CPU产生1个时钟 */
	I2C_DELAY();
	I2C_SCL_L();
	I2C_DELAY();
	I2C_SDA_H();	/* CPU释放SDA总线 */
}

static void i2c_gpio_nack(void)
{
	I2C_SDA_H();	/* CPU驱动SDA = 1 */
	I2C_DELAY();
	I2C_SCL_H();	/* CPU产生1个时钟 */
	I2C_DELAY();
	I2C_SCL_L();
	I2C_DELAY();
}


// ----------------------------------------------------------------------------
// The functions needed for all I2C devices.

void cyg_i2c_stm32_gpio_init(struct cyg_i2c_bus *bus)
{
    if (!init_done) {
        init_done = 1;

        cyg_drv_mutex_init(&i2c_lock);

        /* 配置GPIO */
        I2C_SCL_OUT();
        I2C_SDA_OUT();

    	/* 给一个停止信号, 复位I2C总线上的所有设备到待机模式 */
    	i2c_gpio_stop();
    }
}

cyg_uint32 cyg_i2c_stm32_gpio_tx(const cyg_i2c_device *dev,
                               cyg_bool send_start,
                               const cyg_uint8 *tx_data,
                               cyg_uint32 count,
                               cyg_bool send_stop)
{
    cyg_i2c_stm32_gpio_extra * extra =
        (cyg_i2c_stm32_gpio_extra *)dev->i2c_bus->i2c_extra;

    DEBUG("TX LOCK 0x%X, send_start=%d, send_stop=%d, count=%u, data[0] = 0x%X\n",
                dev->i2c_address, send_start, send_stop, count, tx_data[0]);

    // Mutex for avoiding mulitple accesses to the hardware
    while (!cyg_drv_mutex_lock(&i2c_lock));
    cyg_uint32 bytes_transmitted = 0;

    if (!init_done) {
        diag_printf("I2C ERROR: Init NOT done \n");
        goto err_exit;
    }

    i2c_gpio_start();

    /* 发送设备地址+读写控制bit（0 = w， 1 = r) bit7 先传 */
    i2c_gpio_write8((dev->i2c_address << 1) | 0);
    if (i2c_gpio_wait_ack() == 1) {
        goto tx_exit;
    }

    //  Do the write
    int tx_byte;
    DEBUG("tx_data = ");
    for (tx_byte = count; tx_byte > 0; tx_byte--) {
        DEBUG("0x%X ", *tx_data);
        i2c_gpio_write8(*tx_data);
        if (i2c_gpio_wait_ack() == 1) {
            break; // timeout
        }

        tx_data++; // Point to next data
        bytes_transmitted++; // Ok - one more byte transmitted.
    }
    DEBUG("\n");
    DEBUG("TX UNLOCK 0x%X\n", dev->i2c_address);

tx_exit:
    i2c_gpio_stop();
err_exit:
    cyg_drv_mutex_unlock(&i2c_lock);
    return bytes_transmitted;
}

cyg_uint32 cyg_i2c_stm32_gpio_rx(const cyg_i2c_device* dev,
                               cyg_bool send_start,
                               cyg_uint8* rx_data,
                               cyg_uint32 count,
                               cyg_bool send_nack,
                               cyg_bool send_stop)
{
    cyg_i2c_stm32_gpio_extra * extra =
        (cyg_i2c_stm32_gpio_extra *)dev->i2c_bus->i2c_extra;

    DEBUG("RX LOCK 0x%02X\n",dev->i2c_address);
    // Mutex for avoiding mulitple accesses to the hardware
    while (!cyg_drv_mutex_lock(&i2c_lock));
    cyg_uint32 bytes_recieved = 0;

    if (!init_done) {
        diag_printf("I2C Error: Init NOT done \n");
        goto err_exit;
    }

    if (send_start) {
        i2c_gpio_start();
        /* 发送设备地址+读写控制bit（0 = w， 1 = r) bit7 先传 */
        i2c_gpio_write8((dev->i2c_address << 1) | 1);
        if (i2c_gpio_wait_ack() == 1) {
            goto rx_exit;
        }
    }

    int rx_byte;

    DEBUG("count = %d, read: ", count);

    // Read out the received data
    for (rx_byte = count; rx_byte > 0; rx_byte--) {
        *rx_data = i2c_gpio_read8();
        DEBUG("0x%02X ", *rx_data);
        rx_data++;
        bytes_recieved++;

        if (send_nack && rx_byte == 1) {
            i2c_gpio_nack();
        } else {
            i2c_gpio_ack();
        }
    }
    DEBUG("\n");

rx_exit:
    if (send_stop) {
        i2c_gpio_stop();
    }
err_exit:
    cyg_drv_mutex_unlock(&i2c_lock);
    return bytes_recieved;

}

void cyg_i2c_stm32_gpio_stop(const cyg_i2c_device *dev)
{
}


// -----------------------------------------------------------------------------
// End of file
