//==========================================================================
//
//      wm8978.c
//
//      WM8978 driver for WOLFSON stereo codec with speaker driver
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
// Date:         2013-06-02
// Description:  WM8978 driver for WOLFSON stereo codec with speaker driver
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/devs_sound_i2c_wm8978.h>
#include <pkgconf/io.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_io.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/infra/diag.h>
#include <cyg/io/config_keys.h>
#include <cyg/io/devtab.h>
#include <cyg/io/io.h>

#include <cyg/io/wm8978.h>


// -----------------------------------------------------------------------------
// Diagnostics

#if CYGDAT_DEVS_SOUND_I2C_WM8978_TRACE
#define snd_wm8978_diag( __fmt, ... ) \
 diag_printf("WM8978: %s[%3d]: " __fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__ );
#define snd_wm8978_dump_buf( __addr, __size ) diag_dump_buf( __addr, __size )
#else
#define snd_wm8978_diag( __fmt, ... )
#define snd_wm8978_dump_buf( __addr, __size )
#endif


static bool      snd_wm8978_init(struct cyg_devtab_entry *tab);
static Cyg_ErrNo snd_wm8978_lookup(struct cyg_devtab_entry **tab,
                           struct cyg_devtab_entry *st,
                           const char *name);
static Cyg_ErrNo snd_wm8978_write(cyg_io_handle_t handle,
                           const void *buf,
                           cyg_uint32 *len);
static Cyg_ErrNo snd_wm8978_read(cyg_io_handle_t handle,
                           void *buffer,
                           cyg_uint32 *len);
static Cyg_ErrNo snd_wm8978_set_config(cyg_io_handle_t handle,
                           cyg_uint32 key,
                           const void *buffer,
                           cyg_uint32 *len);
static Cyg_ErrNo snd_wm8978_get_config(cyg_io_handle_t handle,
                           cyg_uint32 key,
                           void *buffer,
                           cyg_uint32 *len);

//-----------------------------------------------------------------------------
// Instantiate Device

#include CYGDAT_DEVS_SOUND_I2C_WM8978_INL


// ----------------------------------------------------------------------------

/*
 * wm8978 has 58 registers in total.
 *
 * R0 is the reset register:
   The WM8978 can be reset by performing a write of any value to the software
   reset register (address 0 hex).  This will cause all register values to be
   reset to their default values.
 *
 *
 */
static cyg_uint16 wm8978_reg_val_buf[] = {
    /* The following are the default values of wm8978 */
	0x000, 0x000, 0x000, 0x000, 0x050, 0x000, 0x140, 0x000,
	0x000, 0x000, 0x000, 0x0FF, 0x0FF, 0x000, 0x100, 0x0FF,
	0x0FF, 0x000, 0x12C, 0x02C, 0x02C, 0x02C, 0x02C, 0x000,
	0x032, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x038, 0x00B, 0x032, 0x000, 0x008, 0x00C, 0x093, 0x0E9,
	0x000, 0x000, 0x000, 0x000, 0x003, 0x010, 0x010, 0x100,
	0x100, 0x002, 0x001, 0x001, 0x039, 0x039, 0x039, 0x039,
	0x001, 0x001
};

static bool wm8978_reg_write(snd_wm8978_dev_t * device,
                                  cyg_uint8 reg, cyg_uint16 val)
{
    cyg_uint8 tx_data[2];

    if (reg >= (sizeof(wm8978_reg_val_buf) / sizeof(wm8978_reg_val_buf[0]))) {
        return  false;
    }
    tx_data[0] = (((reg << 1) & 0xFE) | ((val >> 8) & 0x01));
    tx_data[1] = (cyg_uint8)(val & 0xFF);
    if (cyg_i2c_tx(device->i2c_dev,
                    tx_data, sizeof(tx_data)) != sizeof(tx_data)) {
        snd_wm8978_diag("Write failed, reg: 0x%02X, val: 0x%04X\n", reg, val);
        return  false;
    }
    wm8978_reg_val_buf[reg] = val;
    return true;
}

static cyg_uint16 wm8978_reg_read(snd_wm8978_dev_t * device, cyg_uint8 reg)
{
    if (reg >= (sizeof(wm8978_reg_val_buf) / sizeof(wm8978_reg_val_buf[0]))) {
        return  0xFFFF;
    }
    return  wm8978_reg_val_buf[reg];
}

static bool wm8978_reset(snd_wm8978_dev_t * device)
{
	const cyg_uint16  reg_default[] = {
    	0x000, 0x000, 0x000, 0x000, 0x050, 0x000, 0x140, 0x000,
    	0x000, 0x000, 0x000, 0x0FF, 0x0FF, 0x000, 0x100, 0x0FF,
    	0x0FF, 0x000, 0x12C, 0x02C, 0x02C, 0x02C, 0x02C, 0x000,
    	0x032, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    	0x038, 0x00B, 0x032, 0x000, 0x008, 0x00C, 0x093, 0x0E9,
    	0x000, 0x000, 0x000, 0x000, 0x003, 0x010, 0x010, 0x100,
    	0x100, 0x002, 0x001, 0x001, 0x039, 0x039, 0x039, 0x039,
    	0x001, 0x001
	};

    if (!wm8978_reg_write(device, 0, 0)) {
        return  false;
    }

    cyg_uint16  i;
	for (i = 0; i < sizeof(reg_default) / reg_default[0]; i++) {
		wm8978_reg_val_buf[i] = reg_default[i];
	}

    return  true;
}

/* Set wm8978 CSB/GPIO1 to decide play(H-level, 1) or record(L-level, 0) */
static bool wm8978_set_gpio1_level(snd_wm8978_dev_t * device, bool level)
{
    cyg_uint16 val;
    val = level ? 7 : 6;
    return  wm8978_reg_write(device, 8, val);
}

static bool wm8978_init(snd_wm8978_dev_t * device)
{
    if (!wm8978_reset(device)) return false;
    if (!wm8978_set_gpio1_level(device, 1)) return false;/* Default play */
    return  true;
}

/*
 * \brief Config wm8978 in and out channel.
 *
 * @dac_in  : Enable DAC input for wm8978.
 * @aux_in  : Enable auxiliary input for wm8978.
 * @line_in : Enable line input for wm8978.
 * @speaker : Enable speaker output for wm8978.
 * @ear_out : Enable ear output for wm8978.
 */
static bool wm8978_config_in_out(snd_wm8978_dev_t * device,
        bool dac_in, bool aux_in, bool line_in, bool speaker_out, bool ear_out)
{
	cyg_uint16 val;

	/*	REG 1
		B8		BUFDCOPEN	= x
		B7		OUT4MIXEN	= x
		B6		OUT3MIXEN	= X  耳机输出时，必须设置为1 (for 安富莱开发板)
		B5		PLLEN	= x
		b4`		MICBEN = x
		B3		BIASEN = 1		必须设置为1模拟放大器才工作
		B2		BUFIOEN = x
		B1:0	VMIDSEL = 3  必须设置为非00值模拟放大器才工作
	*/
	val = 0;
	if ((speaker_out == 1) || (ear_out == 1)) {
		val = ((1 << 3) | (3 << 0));
	}
	if (ear_out == 1) {
		val |= (1 << 6);
	}
	wm8978_reg_write(device, 1, val);

	/*	REG 2
		B8		ROUT1EN = 1;	耳机输出通道
		B7		LOUT1EN = 1;	耳机输出通道
		B6		SLEEP = x;
		B5		BOOSTENR = x;
		B4		BOOSTENL = x;
		B3		INPGAENR = x;
		B2		NPPGAENL = x;
		B1		ADCENR = x;
		B0		ADCENL = x;
	*/
	val = 0;
	if (ear_out == 1) {
		val = ((1 << 8) | (1 << 7));
	}
	wm8978_reg_write(device, 2, val);

	/* REG 3
		B8	OUT4EN = 0
		B7	OUT3EN = x;		耳机输出，用于耳机的地线
		B6	LOUT2EN = 1;	扬声器输出通道
		B5	ROUT2EN = 1;	扬声器输出通道
		B4	----   = x
		B3	RMIXEN = 1;		输出MIX, 耳机和扬声器公用
		B2	LMIXEN = 1;		输出MIX, 耳机和扬声器公用
		B1	DACENR = x;		DAC用
		B0	DACENL = x;
	*/
	val = 0;
	if ((speaker_out == 1) || (ear_out == 1)) {
		val |= ((1 << 3) | (1 << 2));
	}
	if (ear_out == 1) {
		val |= (1 << 7);
	}
	if (speaker_out == 1) {
		val |= ((1 << 6) | (1 << 5));
	}
	if (dac_in == 1) {
		val |= ((1 << 1) | (1 << 0));
	}
	wm8978_reg_write(device, 3, val);

	/*
		REG 11,12
		DAC 音量
	*/
	if (dac_in == 1) {
#if 0	/* 此处不要设置音量, 避免切换时音量状态被改变 */
		wm8978_reg_write(device, 11, 255);
		wm8978_reg_write(device, 12, 255 | 0x100);
#endif
	} else {
		;
	}

	/*	REG 43
		B8:6 = 0
		B5	MUTERPGA2INV = 0;	Mute input to INVROUT2 mixer
		B4	INVROUT2 = ROUT2 反相; 用于扬声器推挽输出
		B3:1	BEEPVOL = 7;	AUXR input to ROUT2 inverter gain
		B0	BEEPEN = 1;	1 = enable AUXR beep input

	*/
	val = 0;
	if (speaker_out == 1) {
		val |= (1 << 4);
	}
	if (aux_in == 1) {
		val |= ((7 << 1) | (1 << 0));
	}
	wm8978_reg_write(device, 43, val);

	/* REG 49
		B8:7	0
		B6		DACL2RMIX = x
		B5		DACR2LMIX = x
		B4		OUT4BOOST = 0
		B3		OUT3BOOST = 0
		B2		SPKBOOST = x	SPK BOOST
		B1		TSDEN = 1    扬声器热保护使能（缺省1）
		B0		VROI = 0	Disabled Outputs to VREF Resistance
	*/
	val = 0;
	if (dac_in == 1) {
		val |= ((0 << 6) | (0 << 5));
	}
	if (speaker_out == 1) {
		val |=  ((0 << 2) | (1 << 1));	/* 1.5x增益,  热保护使能 */
	}
	wm8978_reg_write(device, 49, val);

	/*	REG 50    (50是左声道，51是右声道，配置寄存器功能一致) pdf 40页
		B8:6	AUXLMIXVOL = 111	AUX用于FM收音机信号输入
		B5		AUXL2LMIX = 1		Left Auxilliary input to left channel
		B4:2	BYPLMIXVOL			音量
		B1		BYPL2LMIX = 0;		Left bypass path (from the left channel input boost output) to left output mixer
		B0		DACL2LMIX = 1;		Left DAC output to left output mixer
	*/
	val = 0;
	if (aux_in == 1) {
		val |= ((7 << 6) | (1 << 5));
	}
	if (line_in == 1) {
		val |= ((7 << 2) | (1 << 1));
	}
	if (dac_in == 1) {
		val |= (1 << 0);
	}
	wm8978_reg_write(device, 50, val);
	wm8978_reg_write(device, 51, val);

	/*
		REG 52,53	控制EAR音量
		REG 54,55	控制SPK音量

		B8		HPVU		用于同步更新左右声道
		B7		LOUT1ZC = 1  零位切换
		B6		LOUT1MUTE    0表示正常， 1表示静音
	*/
#if 0	/* 此处不要设置音量, 避免应用程序切换输出时，音量状态被改变 */
	if (ear_out == 1) {
		val = (0x3f | (1 << 7));
		wm8978_reg_write(device, 52, val);
		wm8978_reg_write(device, 53, val | (1 << 8));
	} else {
		val = ((1 << 7) | (1 << 6));
		wm8978_reg_write(device, 52, val);
		wm8978_reg_write(device, 53, val | (1 << 8));
	}

	if (speaker_out == 1) {
		val = (0x3f | (1 << 7));
		wm8978_reg_write(device, 54, val);
		wm8978_reg_write(device, 55, val | (1 << 8));
	} else {
		val = ((1 << 7) | (1 << 6));
		wm8978_reg_write(device, 54, val);
		wm8978_reg_write(device, 55, val | (1 << 8));
	}
#endif

	/*	REG 56   OUT3 mixer ctrl
		B6		OUT3MUTE = 1;
		B5		LDAC2OUT3 = 0;
		B4		OUT4_2OUT3
		B3		BYPL2OUT3
		B2		LMIX2OUT3
		B1		LDAC2OUT3
	*/
	wm8978_reg_write(device, 56, (1 <<6));		/**/

	/* 	Softmute enable = 0 */
	if (dac_in == 1) {
		wm8978_reg_write(device, 10, 0);
	}

    return  true;
}

/*
 * \brief Config wm8978 ADC.
 *
 * @mic_in  : Enable MIC input for wm8978.
 * @aux_in  : Enable auxiliary input for wm8978.
 * @line_in : Enable line input for wm8978.
 */
static bool wm8978_config_adc(snd_wm8978_dev_t * device,
                            bool mic_in, bool aux_in, bool line_in)
{
    cyg_uint16 val;

	/* MIC偏置电路设置, pdf 23页 */
	if (mic_in == 1) {
		/* R1 Power management 1 寄存器bit4 MICBEN 用于控制MIC偏置电路使能 */
		/* R1寄存器还有其他控制功能，为了不影响其他功能，我们先读出这个寄存器的值，然后再改写 */
		val = wm8978_reg_read(device, 1);
		val |= (1 << 4);			/* 驻极体话筒需要设置MIC偏置电路使能，动圈话筒不要设置 */
		wm8978_reg_write(device, 1, val);	/* 回写R1寄存器 */

		/* R44 的 Bit8 控制MIC偏置电路电压，0表示0.9*AVDD, 1表示0.6*AVDD
			Bit0 LIP2INPPGA 使能LIP连接到PGA
			Bit0 LIN2INPPGA 使能LIN连接到PGA
			R44寄存器还有其他控制功能，为了不影响其他功能，我们先读出这个寄存器的值，然后再改写
		*/
		val = wm8978_reg_read(device, 44);
		val |= (0 << 4);			/* 根据驻极体话筒的固有增益进行选择偏置电压 */
		val |= ((1 << 1) | (1 << 0));	 /* 使能MIC的2个口线连接到PGA */
		wm8978_reg_write(device, 44, val);	/* 回写R1寄存器 */
	}

	/* R2 寄存器 	pdf  24页
		B8 	ROUT1EN = 1	使能输出ROUT1， 用于录音监听
		B7 	LOUT1EN = 1使能输出LOUT1，用于录音监听
		B6	SLEEP = 0 休眠控制，0表示正常工作，1表示进入休眠降低功耗，
		B5	BOOSTENR = 1 使能右声道自举电路，需要使用输入信道的放大功能时，必须打开自举电路
		B4	BOOSTENL = 1 使能左声道自举电路
		B3  INPPGAENR = 1 使能右声道PGA使能，1表示使能
		B2  INPPGAENL = 1 使能左声道PGA使能，1表示使能
		B1  ADCENR = 1 使能右声道ADC，1表示使能
		B0  ADCENL = 1 使能左声道ADC，1表示使能
	*/
	if ((mic_in == 0) && (aux_in == 0) && (line_in == 0)) {
		val = 0;
	} else {
		val = (1 << 8) | (1 << 7) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
	}
	wm8978_reg_write(device, 2, val);	/* 写R2寄存器 */

	/*
		Mic 输入信道的增益由 PGABOOSTL 和 PGABOOSTR 控制
		Aux 输入信道的输入增益由 AUXL2BOOSTVO[2:0] 和 AUXR2BOOSTVO[2:0] 控制
		Line 输入信道的增益由 LIP2BOOSTVOL[2:0] 和 RIP2BOOSTVOL[2:0] 控制
	*/
	/*	pdf 21页，R47（左声道），R48（由声道）, MIC防盗器增益控制寄存器
		R47 (R48定义与此相同)
		B8		PGABOOSTL	= 1, 0表示MIC信号直通无增益，1表示MIC信号+20dB增益（通过自举电路）
		B7		= 0， 保留
		B6:4	L2_2BOOSTVOL = x，0表示禁止，1-7表示增益-12dB ~ +6dB  （可以衰减也可以放大）
		B3		= 0， 保留
		B2:0`	AUXL2BOOSTVOL = x，0表示禁止，1-7表示增益-12dB ~ +6dB  （可以衰减也可以放大）
	*/
	val = 0;
	if (mic_in == 1) {
		val |= (1 << 8);	/* MIC增益取+20dB */
	}
	if (aux_in == 1) {
		val |= (3 << 4);	/* Aux增益固定取3，用户可以自行调整 */
	}
	if (line_in == 1) {
		val |= (3 << 0);	/* Line增益固定取3，用户可以自行调整 */
	}
	wm8978_reg_write(device, 47, val);	/* 写左声道输入增益控制寄存器 */
	wm8978_reg_write(device, 48, val);	/* 写右声道输入增益控制寄存器 */

	/* 设置高通滤波器（可选的） pdf 24、25页,
		Bit8 	HPFEN = x，高通滤波器使能，0表示禁止，1表示使能
		BIt7 	HPFAPP = x，选择音频模式或应用模式，0表示音频模式，
		Bit6:4	HPFCUT = x，000-111选择应用模式的截止频率
		Bit3 	ADCOSR 控制ADC过采样率，0表示64x（功耗低） 1表示128x（高性能）
		Bit2   	保留，填0
		Bit1 	ADCPOLR 控制右声道反相使能，1表示使能
		Bit0 	ADCPOLL 控制左声道反相使能，1表示使能
	*/
	val = (1 << 3);			/* 选择高通滤波器禁止，不反相，高性能采样率 */
	wm8978_reg_write(device, 14, val);	/* 回写R14寄存器 */


	/* 设置陷波滤波器（notch filter），主要用于抑制话筒声波正反馈，避免啸叫，此处不使用
		R27，R28，R29，R30 用于控制限波滤波器，pdf 26页
		R7的 Bit7 NFEN = 0 表示禁止，1表示使能
	*/
	val = (0 << 7);
	wm8978_reg_write(device, 27, val);	/* 写寄存器 */
	val = 0;
	wm8978_reg_write(device, 28, val);	/* 写寄存器,填0，因为已经禁止，所以也可不做 */
	wm8978_reg_write(device, 29, val);	/* 写寄存器,填0，因为已经禁止，所以也可不做 */
	wm8978_reg_write(device, 30, val);	/* 写寄存器,填0，因为已经禁止，所以也可不做 */

	/* 数字ADC音量控制，pdf 27页
		R15 控制左声道ADC音量，R16控制右声道ADC音量
		Bit8 	ADCVU  = 1 时才更新，用于同步更新左右声道的ADC音量
		Bit7:0 	增益选择； 0000 0000 = 静音
						   0000 0001 = -127dB
						   0000 0010 = -12.5dB  （0.5dB 步长）
						   1111 1111 = 0dB  （不衰减）
	*/
	val = 0xFF;
	wm8978_reg_write(device, 15, val);	/* 选择0dB，先缓存左声道 */
	val = 0x1FF;
	wm8978_reg_write(device, 16, val);	/* 同步更新左右声道 */

#if 0	/* 通过wm8978_SetMicGain函数设置PGA增益 */
	/*
		PGA 音量控制  R45， R46   pdf 19页
	*/
	val = (16 << 0);
	wm8978_reg_write(device, 45, val);
	wm8978_reg_write(device, 46, val | (1 << 8));
#endif

	/* 自动增益控制 ALC, R32  pdf 19页 */
	val = wm8978_reg_read(device, 32);
	val &= ~((1 << 8) | (1 << 7));	/* 禁止自动增益控制 */
	wm8978_reg_write(device, 32, val);

    return  true;
}

/*
 * \brief Config wm8978 i2s interface.
 *
 * @standard : I2S standard selection for wm8978.
 * @data_len : I2S data length to be transferred for wm8978.
 * @mode     : I2S configuration mode for wm8978.
 */
static bool wm8978_config_audio_if(snd_wm8978_dev_t * device,
                  cyg_uint8 standard, cyg_uint8 data_len, cyg_uint8 mode)
{
	cyg_uint16 val;

	/* pdf 67页，寄存器列表 */

	/*	REG R4, 音频接口控制寄存器
		B8		BCP	 = X, BCLK极性，0表示正常，1表示反相
		B7		LRCP = x, LRC时钟极性，0表示正常，1表示反相
		B6:5	WL = x， 字长，00=16bit，01=20bit，10=24bit，11=32bit （右对齐模式只能操作在最大24bit)
		B4:3	FMT = x，音频数据格式，00=右对齐，01=左对齐，10=I2S格式，11=PCM
		B2		DACLRSWAP = x, 控制DAC数据出现在LRC时钟的左边还是右边
		B1 		ADCLRSWAP = x，控制ADC数据出现在LRC时钟的左边还是右边
		B0		MONO	= 0，0表示立体声，1表示单声道，仅左声道有效
	*/
	val = 0;
	if (standard == I2S_STANDARD_PHILLIPS) {    /* I2S飞利浦标准 */
		val |= (2 << 3);
	} else if (standard == I2S_STANDARD_MSB) {	/* MSB对齐标准(左对齐) */

		val |= (1 << 3);
	} else if (standard == I2S_STANDARD_LSB) {	/* LSB对齐标准(右对齐) */
		val |= (0 << 3);
	} else {	/* PCM标准(16位通道帧上带长或短帧同步或者16位数据帧扩展为32位通道帧) */
		val |= (3 << 3);;
	}

	if (data_len == 24)	{
		val |= (2 << 5);
	} else if (data_len == 32) {
		val |= (3 << 5);
	} else {
		val |= (0 << 5);		/* 16bit */
	}
	wm8978_reg_write(device, 4, val);

	/* R5  pdf 57页 */


	/*
		R6，时钟产生控制寄存器
		MS = 0,  WM8978被动时钟，由MCU提供MCLK时钟
	*/
	wm8978_reg_write(device, 6, 0x000);

	/* 如果是放音则需要设置  WM_GPIO1 = 1 ,如果是录音则需要设置WM_GPIO1 = 0 */
	if (mode == I2S_MODE_MASTER_SENDER) {
		wm8978_set_gpio1_level(device, 1);	/* 驱动WM8978的GPIO1引脚输出1, 用于放音 */
	} else {
		wm8978_set_gpio1_level(device, 0);	/* 驱动WM8978的GPIO1引脚输出0, 用于录音 */
	}

    return  true;
}

static bool wm8978_config_audio_if_channel(snd_wm8978_dev_t * device,
                                              cyg_uint8 channel)
{
    cyg_uint16 val = wm8978_reg_read(device, 4);

	/*	REG R4, 音频接口控制寄存器
		B8		BCP	 = X, BCLK极性，0表示正常，1表示反相
		B7		LRCP = x, LRC时钟极性，0表示正常，1表示反相
		B6:5	WL = x， 字长，00=16bit，01=20bit，10=24bit，11=32bit （右对齐模式只能操作在最大24bit)
		B4:3	FMT = x，音频数据格式，00=右对齐，01=左对齐，10=I2S格式，11=PCM
		B2		DACLRSWAP = x, 控制DAC数据出现在LRC时钟的左边还是右边
		B1 		ADCLRSWAP = x，控制ADC数据出现在LRC时钟的左边还是右边
		B0		MONO	= 0，0表示立体声，1表示单声道，仅左声道有效
	*/

    val = (val & (~(VALUE_(0, 7)) )  );

    if (channel == SND_CARD_PARAM_CHANNEL_STEREO) {
        ;/* NULL op */
    } else if (channel == SND_CARD_PARAM_CHANNEL_MONO_L_CH) {
        val |= 0x01;
    } else if (channel == SND_CARD_PARAM_CHANNEL_MONO_R_CH) {
        val |= 0x07;
    } else {
        return  false;
    }

    return  wm8978_reg_write(device, 4, val);
}

static bool wm8978_config_audio_if_standard(snd_wm8978_dev_t * device,
                                              cyg_uint8 standard)
{
    cyg_uint16 val = 0;

	/*	REG R4, 音频接口控制寄存器
		B8		BCP	 = X, BCLK极性，0表示正常，1表示反相
		B7		LRCP = x, LRC时钟极性，0表示正常，1表示反相
		B6:5	WL = x， 字长，00=16bit，01=20bit，10=24bit，11=32bit （右对齐模式只能操作在最大24bit)
		B4:3	FMT = x，音频数据格式，00=右对齐，01=左对齐，10=I2S格式，11=PCM
		B2		DACLRSWAP = x, 控制DAC数据出现在LRC时钟的左边还是右边
		B1 		ADCLRSWAP = x，控制ADC数据出现在LRC时钟的左边还是右边
		B0		MONO	= 0，0表示立体声，1表示单声道，仅左声道有效
	*/

	val = wm8978_reg_read(device, 4);
    snd_wm8978_diag("val = 0x%04X\n", val);
    val = (val & (~(VALUE_(3, 3)) )  );
    snd_wm8978_diag("val = 0x%04X\n", val);

	if (standard == I2S_STANDARD_PHILLIPS) {    /* I2S飞利浦标准 */
		val |= (2 << 3);
	} else if (standard == I2S_STANDARD_MSB) {	/* MSB对齐标准(左对齐) */
		val |= (1 << 3);
	} else if (standard == I2S_STANDARD_LSB) {	/* LSB对齐标准(右对齐) */
		val |= (0 << 3);
	} else {	/* PCM标准(16位通道帧上带长或短帧同步或者16位数据帧扩展为32位通道帧) */
		val |= (3 << 3);
	}
    snd_wm8978_diag("standard = %d, val = 0x%04X\n", standard, val);
	return  wm8978_reg_write(device, 4, val);
}

static bool wm8978_config_audio_if_data_len(snd_wm8978_dev_t * device,
                                              cyg_uint8 data_len)
{
    cyg_uint16 val = 0;

	/*	REG R4, 音频接口控制寄存器
		B8		BCP	 = X, BCLK极性，0表示正常，1表示反相
		B7		LRCP = x, LRC时钟极性，0表示正常，1表示反相
		B6:5	WL = x， 字长，00=16bit，01=20bit，10=24bit，11=32bit （右对齐模式只能操作在最大24bit)
		B4:3	FMT = x，音频数据格式，00=右对齐，01=左对齐，10=I2S格式，11=PCM
		B2		DACLRSWAP = x, 控制DAC数据出现在LRC时钟的左边还是右边
		B1 		ADCLRSWAP = x，控制ADC数据出现在LRC时钟的左边还是右边
		B0		MONO	= 0，0表示立体声，1表示单声道，仅左声道有效
	*/

	val = wm8978_reg_read(device, 4);
    snd_wm8978_diag("val = 0x%04X\n", val);
    val = (val & (~(VALUE_(5, 3)) )  );
    snd_wm8978_diag("val = 0x%04X\n", val);

	if (data_len == 24)	{
		val |= (2 << 5);
	} else if (data_len == 32) {
		val |= (3 << 5);
	} else {
		val |= (0 << 5);		/* 16bit */
	}
    snd_wm8978_diag("data_len = %d, val = 0x%04X\n", data_len, val);
	return  wm8978_reg_write(device, 4, val);
}

static bool wm8978_config_audio_if_mode(snd_wm8978_dev_t * device,
                                              cyg_uint8 mode)
{
	/* 如果是放音则需要设置  WM_GPIO1 = 1 ,如果是录音则需要设置WM_GPIO1 = 0 */
	if (mode == I2S_MODE_MASTER_SENDER) {
		wm8978_set_gpio1_level(device, 1);/* 驱动WM8978的GPIO1引脚输出1, 用于放音 */
	} else {
		wm8978_set_gpio1_level(device, 0);/* 驱动WM8978的GPIO1引脚输出0, 用于录音 */
	}

    return  true;
}

static bool wm8978_power_down(snd_wm8978_dev_t * device)
{
	/*
	Set DACMU = 1 to mute the audio DACs.
	Disable all Outputs.
	Disable VREF and VMIDSEL.
	Switch off the power supplies
	*/
	cyg_uint16 val;

	val = wm8978_reg_read(device, 10);
	val |= (1u <<6);
	wm8978_reg_write(device, 10, val);

	/* 未完 */

    return  true;
}

static bool wm8978_set_mic_gain(snd_wm8978_dev_t * device, cyg_uint8 gain)
{
	if (gain > WM8978_GAIN_MAX) {
		gain = WM8978_GAIN_MAX;
	}

	/* PGA 音量控制  R45， R46   pdf 19页
		Bit8	INPPGAUPDATE
		Bit7	INPPGAZCL		过零再更改
		Bit6	INPPGAMUTEL		PGA静音
		Bit5:0	增益值，010000是0dB
	*/
	wm8978_reg_write(device, 45, gain);
	wm8978_reg_write(device, 46, gain | (1 << 8));

    return  true;
}

static bool wm8978_mute(snd_wm8978_dev_t * device, bool mute)
{
    cyg_uint16 val;

	if (mute == 1) { /* 静音 */
		val = wm8978_reg_read(device, 52); /* Left Mixer Control */
		val |= (1u << 6);
		wm8978_reg_write(device, 52, val);

		val = wm8978_reg_read(device, 53); /* Left Mixer Control */
		val |= (1u << 6);
		wm8978_reg_write(device, 53, val);

		val = wm8978_reg_read(device, 54); /* Right Mixer Control */
		val |= (1u << 6);
		wm8978_reg_write(device, 54, val);

		val = wm8978_reg_read(device, 55); /* Right Mixer Control */
		val |= (1u << 6);
		wm8978_reg_write(device, 55, val);
	} else {	/* 取消静音 */
		val = wm8978_reg_read(device, 52);
		val &= ~(1u << 6);
		wm8978_reg_write(device, 52, val);

		val = wm8978_reg_read(device, 53); /* Left Mixer Control */
		val &= ~(1u << 6);
		wm8978_reg_write(device, 53, val);

		val = wm8978_reg_read(device, 54);
		val &= ~(1u << 6);
		wm8978_reg_write(device, 54, val);

		val = wm8978_reg_read(device, 55); /* Left Mixer Control */
		val &= ~(1u << 6);
		wm8978_reg_write(device, 55, val);
	}

    return  true;
}


static cyg_uint8 wm8978_get_volume(snd_wm8978_dev_t * device)
{
    return (cyg_uint8)(wm8978_reg_read(device, 52) & 0x3F);
}

static bool wm8978_set_volume(snd_wm8978_dev_t * device,
                        cyg_uint8 left_volume, cyg_uint8 right_volume)
{
	cyg_uint16 regL;
	cyg_uint16 regR;

	if (left_volume > 0x3F)	{
		left_volume = 0x3F;
	}
	if (right_volume > 0x3F)	{
		right_volume = 0x3F;
	}
	regL = left_volume;
	regR = right_volume;

	/* 先更新左声道缓存值 */
	wm8978_reg_write(device, 52, regL | 0x00);

	/* 再同步更新左右声道的音量 */
	wm8978_reg_write(device, 53, regR | 0x100);	/* 0x180表示 在音量为0时再更新，避免调节音量出现的“嘎哒”声 */

	/* 先更新左声道缓存值 */
	wm8978_reg_write(device, 54, regL | 0x00);

	/* 再同步更新左右声道的音量 */
	wm8978_reg_write(device, 55, regR | 0x100);	/* 在音量为0时再更新，避免调节音量出现的“嘎哒”声 */

    return  true;
}

// ----------------------------------------------------------------------------
// Macro utilities

// 配置wm8978硬件,MIC输入到ADC,准备录音
#define WM8978_MIC_2_ADC(device)        \
        wm8978_config_adc(device, MIC_ON, AUX_OFF, LINE_ON)

/* Config wm8978 audio input and audio output */

// 配置wm8978硬件,输入混音（AUX,LINE,MIC）直接输出到扬声器
#define WM8978_MIC_2_SPEAKER(device)    \
        wm8978_config_in_out(device, DAC_OFF, AUX_OFF, LINE_ON, SPK_ON, EAR_OFF)
// 配置wm8978硬件,输入混音（AUX,LINE,MIC）直接输出到耳机
#define WM8978_MIC_2_EAR(device)        \
        wm8978_config_in_out(device, DAC_OFF, AUX_OFF, LINE_ON, SPK_OFF, EAR_ON)

// 初始化wm8978硬件设备,Aux(FM收音机)输出到扬声器
#define WM8978_AUX_2_SPEAKER(device)    \
        wm8978_config_in_out(device, DAC_OFF, AUX_ON, LINE_OFF, SPK_ON, EAR_OFF)
// 初始化wm8978硬件设备,Aux(FM收音机)输出到耳机
#define WM8978_AUX_2_EAR(device)        \
        wm8978_config_in_out(device, DAC_OFF, AUX_ON, LINE_OFF, SPK_OFF, EAR_ON)

// 初始化wm8978硬件设备,DAC输出到扬声器
#define WM8978_DAC_2_SPEAKER(device)    \
        wm8978_config_in_out(device, DAC_ON, AUX_OFF, LINE_OFF, SPK_ON, EAR_OFF)
// 初始化wm8978硬件设备,DAC输出到耳机
#define WM8978_DAC_2_EAR(device)        \
        wm8978_config_in_out(device, DAC_ON, AUX_OFF, LINE_OFF, SPK_OFF, EAR_ON)



// ----------------------------------------------------------------------------

static bool snd_wm8978_init(struct cyg_devtab_entry *tab)
{
    snd_wm8978_dev_t * snd_dev = (snd_wm8978_dev_t *)tab->priv;
    snd_dev->init = 0;
    return true;
}

static
Cyg_ErrNo snd_wm8978_lookup(struct cyg_devtab_entry **tab,
                                struct cyg_devtab_entry *st,
                                const char *name)
{
    cyg_io_handle_t chan = (cyg_io_handle_t)st;
    snd_wm8978_dev_t * snd_dev = (snd_wm8978_dev_t *)(*tab)->priv;

    if (!snd_dev->init) {
        snd_dev->dev_handle = chan;

        if (!wm8978_init(snd_dev)) {
            diag_printf("wm8978 init failed!");
        }

        wm8978_set_volume(snd_dev, 39, 39); /* 缺省音量 */
        wm8978_set_mic_gain(snd_dev, 24);   /* 缺省PGA增益 */

        wm8978_config_audio_if(snd_dev,
                           I2S_STANDARD_PHILLIPS, 16, I2S_MODE_MASTER_SENDER);
        snd_dev->init = 1;
    }

    return ENOERR;
}

static
Cyg_ErrNo snd_wm8978_write(cyg_io_handle_t handle,
                             const void *buf,
                             cyg_uint32 *len)
{
    cyg_devtab_entry_t * t = (cyg_devtab_entry_t *)handle;
    snd_wm8978_dev_t * snd_dev = (snd_wm8978_dev_t *)t->priv;
    cyg_io_handle_t chan = snd_dev->dev_handle;
    return  cyg_io_write(chan, buf, len);
}

static
Cyg_ErrNo snd_wm8978_read(cyg_io_handle_t handle,
                             void *buffer,
                             cyg_uint32 *len)
{
    cyg_devtab_entry_t * t = (cyg_devtab_entry_t *)handle;
    snd_wm8978_dev_t * snd_dev = (snd_wm8978_dev_t *)t->priv;
    cyg_io_handle_t chan = snd_dev->dev_handle;
    return  cyg_io_read(chan, buffer, len);
}


static
Cyg_ErrNo snd_wm8978_set_role_out(snd_wm8978_dev_t * snd_dev)
{
    snd_wm8978_diag("snd_dev->role = %d, snd_dev->output = %d\n",
                     snd_dev->role, snd_dev->output);

    if (snd_dev->role == SOUND_CARD_PARAM_ROLE_RECORD) {
        if (snd_dev->output == SOUND_CARD_PARAM_OUTPUT_SPEAKER) {
            WM8978_MIC_2_SPEAKER(snd_dev);  // 录音的同时输出到扬声器
        } else if (snd_dev->output == SOUND_CARD_PARAM_OUTPUT_EAR) {
            WM8978_MIC_2_EAR(snd_dev);      // 录音的同时输出到耳机
        } else ;/* Don't output */

        // 必须放在后上述代码段的后面，否则不能采集声音
    	WM8978_MIC_2_ADC(snd_dev);/* 配置ADC采集MIC通道 */

    } else if (snd_dev->role == SOUND_CARD_PARAM_ROLE_PLAYER) {
        if (snd_dev->output == SOUND_CARD_PARAM_OUTPUT_SPEAKER) {
            WM8978_DAC_2_SPEAKER(snd_dev);  // 放音输出到扬声器
        } else if (snd_dev->output == SOUND_CARD_PARAM_OUTPUT_EAR) {
            WM8978_DAC_2_EAR(snd_dev);      // 放音输出到耳机
        } else {
            return -EINVAL;
        }
    } else if (snd_dev->role == SOUND_CARD_PARAM_ROLE_AUX) {
        if (snd_dev->output == SOUND_CARD_PARAM_OUTPUT_SPEAKER) {
            WM8978_AUX_2_SPEAKER(snd_dev);  // AUX输出到扬声器
        } else if (snd_dev->output == SOUND_CARD_PARAM_OUTPUT_EAR) {
            WM8978_AUX_2_EAR(snd_dev);      // AUX输出到耳机
        } else {
            return -EINVAL;
        }
    } else {
        return -EINVAL;
    }

    return ENOERR;
}

static
Cyg_ErrNo snd_wm8978_set_config(cyg_io_handle_t handle,
                                    cyg_uint32 key,
                                    const void *buffer,
                                    cyg_uint32 *len)
{
    cyg_devtab_entry_t * t = (cyg_devtab_entry_t *)handle;
    snd_wm8978_dev_t * snd_dev = (snd_wm8978_dev_t *)t->priv;
    cyg_io_handle_t chan = snd_dev->dev_handle;
    cyg_snd_card_volume_t * vol;
    cyg_uint32 data = *((cyg_uint32 *)buffer);
    cyg_uint32 size = *len;
    Cyg_ErrNo res = ENOERR;

    if (size < 4) return -EINVAL;
    snd_wm8978_diag("data = 0x%08X, len = %d\n", data, size);

    switch (key) {
    case CYG_IO_SET_CONFIG_SND_CARD_SWITCH:     /* Sound card switch */
        res = cyg_io_set_config(chan, CYG_IO_SET_CONFIG_I2S_SWITCH, buffer, len);
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_MUTE:      /* Sound card mute enable or disable */
        if (!wm8978_mute(snd_dev, (data ? 1 : 0))) {
            return  -EIO;
        }
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_AUDIO_FREQ: /* Sound audio frequency */
        res = cyg_io_set_config(chan, CYG_IO_SET_CONFIG_I2S_AUDIO_FREQ, buffer, len);
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_STANDARD:   /* Sound audio standard */
        if (!wm8978_config_audio_if_standard(snd_dev, (cyg_uint8)data)) {
            return  -EIO;
        }
        res = cyg_io_set_config(chan, CYG_IO_SET_CONFIG_I2S_STANDARD, buffer, len);
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_DATA_LEN:   /* Sound audio data len */
        if (!wm8978_config_audio_if_data_len(snd_dev, (cyg_uint8)data)) {
            return  -EIO;
        }
        res = cyg_io_set_config(chan, CYG_IO_SET_CONFIG_I2S_DATA_LEN, buffer, len);
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_VOLUME:     /* Sound left and right volume */
        vol = (cyg_snd_card_volume_t *)buffer;
        wm8978_set_volume(snd_dev, (cyg_uint8)(vol->left_volume  & 0xFF),
                                   (cyg_uint8)(vol->right_volume & 0xFF)   );
        snd_wm8978_diag("left_volume = %d, right_volume = %d\n",
                         (cyg_uint8)(vol->left_volume  & 0xFF),
                         (cyg_uint8)(vol->right_volume & 0xFF));
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_MIC_GAIN:   /* Sound MIC gain only for recorder */
        snd_wm8978_diag("MIC gain = %d\n", (cyg_uint8)data);
        wm8978_set_mic_gain(snd_dev, (cyg_uint8)data);
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_ROLE:       /* Sound card input */
        if (data == SOUND_CARD_PARAM_ROLE_PLAYER ||
            data == SOUND_CARD_PARAM_ROLE_AUX) {
            snd_dev->role = (cyg_uint8)(data & 0xFF);
            data = I2S_MODE_MASTER_SENDER;
        } else if (data == SOUND_CARD_PARAM_ROLE_RECORD) {
            snd_dev->role = (cyg_uint8)(data & 0xFF);
            data = I2S_MODE_MASTER_RECEIVER;
        } else {
            return -EINVAL;
        }

        if ((res = snd_wm8978_set_role_out(snd_dev)) != ENOERR) {
            return  res;
        }
        if (!wm8978_config_audio_if_mode(snd_dev, (cyg_uint8)data)) {
            return  -EIO;
        }
        size = sizeof(data);
        res = cyg_io_set_config(chan, CYG_IO_SET_CONFIG_I2S_MODE, &data, &size);
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_OUTPUT:     /* Sound card output */
        snd_dev->output = (cyg_uint8)data;
        res = snd_wm8978_set_role_out(snd_dev);
        break;

    case CYG_IO_SET_CONFIG_SND_CARD_CHANNEL:    /* Sound card channel */
        if (!wm8978_config_audio_if_channel(snd_dev, (cyg_uint8)data)) {
            return -EINVAL;
        }
        break;

    default:
        return -EINVAL;
    }

    return res;
}

static
Cyg_ErrNo snd_wm8978_get_config(cyg_io_handle_t handle,
                                    cyg_uint32 key,
                                    void *buffer,
                                    cyg_uint32 *len)
{
    return ENOERR;
}


// -----------------------------------------------------------------------------
// End of file
