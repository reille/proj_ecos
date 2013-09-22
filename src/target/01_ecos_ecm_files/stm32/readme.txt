/*****************************************************************************
**
** 		         eCos配置文件版本说明(历史记录)
**
** Copyright (C) 2013 eCos技术网(http://www.52ecos.net). All Rights Reserved.
**
** Author: reille
** Date  : 2013.04.06
**
*****************************************************************************/


==============================================================================

[eCos] Ver0.21: 2013.08.24
-------------------------

在[eCos] Ver0.20的基础上: 使能LWIP协议栈配置项：CYGPKG_LWIP_DNS
原因：移植goahead2.5时需要用到gethostbyname()函数，而这函数又依赖DNS的实现。




==============================================================================

[eCos] Ver0.20: 2013.07.28
-------------------------

在[eCos] Ver0.19的基础上: 添加CYGPKG_CPULOAD包






==============================================================================

[eCos] Ver0.19: 2013.07.28
-------------------------

在[eCos] Ver0.18的基础上: 

1. 禁止配置项CYGDBG_IO_DISK_DEBUG，即关闭eCos中Disk IO驱动包的调试信息；
2. 禁止配置项CYGPKG_DEVS_DISK_TESTING；
3. 使能配置项CYGSEM_FILEIO_INFO_DISK_USAGE；




==============================================================================

[eCos] Ver0.18: 2013.07.23
-------------------------

在[eCos] Ver0.17的基础上: 使能配置项CYGDBG_IO_DISK_DEBUG，即开启eCos中Disk IO驱动包的调试信息。




==============================================================================

[eCos] Ver0.17: 2013.07.18
-------------------------

在[eCos] Ver0.16的基础上: 

主要是添加了STM32 SD卡驱动相关组件包，如下所述：

1. 与SD卡驱动相关的组件：
>> CYGPKG_DEVS_DISK_CORTEXM_STM32_MMC — STM32 SD卡驱动即STM32 SDIO驱动；同时设置该驱动组件 CYGNUM_DEVS_DISK_CORTEXM_STM32_MMC_DISK0_BBUF_SIZE 配置项为512；
>>> CYGPKG_IO_DISK — STM32 SD卡驱动 依赖的IO层驱动组件；

2. 与应用相关的组件：
>>> CYGPKG_FS_FAT — FAT Filesystem组件包；
>>> CYGPKG_BLOCK_LIB — CYGPKG_FS_FAT 依赖的组件包；
>>> CYGPKG_LINUX_COMPAT — CYGPKG_BLOCK_LIB 依赖的组件包；



==============================================================================

[eCos] Ver0.16: 2013.06.21
-------------------------

在[eCos] Ver0.15的基础上: 

1. 禁止CYGPKG_DEVS_SOUND_I2S_CORTEXM_STM32包的Driver trace；

2. 禁止CYGPKG_DEVS_SOUND_I2C_WM8978包的Driver trace；



==============================================================================

[eCos] Ver0.15: 2013.06.11
-------------------------

在[eCos] Ver0.14的基础上: 

1. 添加了CYGPKG_DEVS_SOUND_I2S_CORTEXM_STM32包，并使能了ST STM32 I2S Bus 2配置项，同时配置该配置项的Bounce buffer size为1024；

2. 添加了CYGPKG_DEVS_SOUND_I2C_WM8978包；



==============================================================================

[eCos] Ver0.14: 2013.05.31
-------------------------

在[eCos] Ver0.13的基础上: 

把adc1.inl还原为最新，并在图形配置工具中打开了ADC1的ADC channel14和ADC channel16（其它ADC配置采用默认配置）。



==============================================================================

[eCos] Ver0.13: 2013.05.26
-------------------------

在[eCos] Ver0.12的基础上: 

1>. 设置CYGNUM_DEVS_SPI_CORTEXM_STM32_BUS1_BBUF_SIZE配置项为256，原先是0；

2>. 设置CYGNUM_DEVS_FLASH_SPI_M25PXX_READ_BLOCK_SIZE配置项为256，原先是0；


但是这样配置后，工程编译后得到的bin文件有异常，大小变为了4096个字节。这个问题以前也出现过，暂时不知道什么原因引起的。注：得到的srec格式映像文件是正常的。

------------
2013.05.31注：

这个问题的原因：当CYGNUM_DEVS_SPI_CORTEXM_STM32_BUS1_BBUF_SIZE大小设置为大于0时，会把一个DMA要用到的数据缓存放到sram段，在使用objcopy工具把elf转换成bin时，objcopy把从sram段开始地址到bss段结束地址这段空间的数据拷贝到目标bin文件中，elf的sram段开始地址(0x20000400)到bss段结束地址(0x68021390)这段空间是不连续的而且很大，所以导致生成的bin文件不正常。

解决方法：在转换成bin文件时，加上选项：-R .sram，即在转换时，把sram段去除(链接生成的bin文件实际上不需要它)

在[eCos] Ver0.4版本中，关于ADC配置出现的bin问题，也正是由于此原因造成的。



==============================================================================

[eCos] Ver0.12: 2013.05.25
-------------------------

在[eCos] Ver0.10的基础上: 添加了I2C驱动，不过这个I2C驱动是基于GPIO模拟的。



==============================================================================

[eCos] Ver0.11: 2013.05.24
-------------------------

在[eCos] Ver0.10的基础上: 添加了I2C驱动



==============================================================================

[redboot] Ver0.2: 2013.05.19
----------------------------
1> 在图形配置工具中，使用eCos源码包Template (Hardware：ST STM32F10XXX platform board，Packages：redboot)，然后导入（import）redboot_stm32f103xx_Ver0.1.ecm最小配置文件，随后去除Framebuffer support软件包，解决相关冲突。

2> 配置Redboot Networking配置项：
关闭Support HTTP for download配置项，配置Default IP address（注意IP地址用逗号隔开）为192.168.0.24，关闭Enable BOOTP/DHCP support配置项，配置默认网关和IP掩码分别为：192.168.0.1和255.255.255.0（与IP配置值一样，在配置项值里用逗号隔开）。

3> 关闭Allows RedBoot to suppor disks配置项。

编译出来的redboot，可用内存范围为：0x68007aa8-0x680dd000；
可以使用tftp下载映像文件。

注意，上电或复位，redboot启动时会有小小延迟。


!!!发现存在的一些问题!!!

1>. 直接在工程中执行make redboot，发现编译出来的redboot.bin不带网络功能；
2>. 在图形配置工具中编译出来的redboot.bin，发现在设置启动脚本时不能设置IP等，出现：“Getting config information in READONLY mode”提示；而用IP命令去设置IP、掩码、tftp服务器IP等参数时，不能掉电保存。



==============================================================================

[eCos] Ver0.10: 2013.05.12
-------------------------

在[eCos] Ver0.8的基础上: 去除了DHCP协议的支持，配置项为：CYGPKG_LWIP_DHCP



==============================================================================

[eCos] Ver0.9: 2013.05.11
-------------------------

在[eCos] Ver0.8的基础上: 使能了lwip的Debugging配置项：CYGDBG_LWIP_DEBUG；


==============================================================================

[eCos] Ver0.8: 2013.05.11
-------------------------

在[eCos] Ver0.7的基础上:

1> 添加了LWIP包：CYGPKG_NET_LWIP；
2> 同时配置LWIP为Sequential mode（默认是Simple mode），让LWIP支持多线程，同时使LWIP的API支持标准的socket的编程。
3> 配置eth0 configuration选项，取消“Use DHCP”，并在Network configuration中配置默认的IP地址、掩码和网关； 


==============================================================================

[eCos] Ver0.7: 2013.05.10
-------------------------
在[eCos] Ver0.6的基础上:

主要添加了与网卡（DM9000）相关的包：
 CYGPKG_IO_ETH_DRIVERS
 CYGPKG_DEVS_ETH_CORTEXM_STM32_STM32F10XXX
 CYGPKG_DEVS_ETH_DAVICOM_DM9000

注：使用[eCos] Ver0.7的ecc配置文件，在eCos的图形配置工具中编译时不会有问题，但在工程中，使用ecosconfig工具import本[eCos] Ver0.7的ecm文件，编译时会有问题：
“在eth_drv.h文件168行，会报sc_arpcom这个变量类型问题”。对于这个编译问题，添加LWIP包可以解决。


==============================================================================

[eCos] Ver0.6: 2013.05.04
-------------------------
在[eCos] Ver0.5的基础上:

1> 使能ST STM32 SPI bus 1 总线，配置项为：CYGHWR_DEVS_SPI_CORTEXM_STM32_BUS1，并配置该SPI1片选为：“SPI_CS(B, 2), SPI_CS(G, 11)”，其中，SPI_CS(B, 2)是用于SPI flash的片选信号线；SPI_CS(G, 11)是用于触摸屏的片选信号线，注意位置不能随意颠倒，否则会影响真正被片选的芯片，详见spi_stm32.c文件中167行中关于CYGHWR_DEVS_SPI_CORTEXM_STM32_BUS1_CS_GPIOS宏的使用。

2> 添加File IO即CYGPKG_IO_FILEIO软件包；

3> 添加“Touch screen driver for the TSC2046 controller”包。



==============================================================================

[eCos] Ver0.5: 2013.05.01
-------------------------
在[eCos] Ver0.4的基础上，增加了CYGPKG_IO_FRAMEBUF包和STM32 LCD的驱动包。



==============================================================================

[eCos] Ver0.4: 2013.04.24
-------------------------
[eCos] Ver0.3版本在配置时打开了ADC1的ADC channel14和ADC channel16（其它ADC配
置采用默认配置），但是生成bin格式映像文件却发生了异常（srec格式OK）：经反复实
验，只要在图形配置工具中打开了ADC1的ADC channel14和ADC channel16，则不能生成
正常的bin格式映像文件；相反，则要不打开这两个channel（或者其它channel），则可
以生成正常的bin文件。为解决此问题，不在图形配置工具中打开channel，而是直接在
驱动代码adc1.inl源文件中定义，这样也可以生成正常的bin文件。添加的代码如下：
// Added start by reille 2013.04.24
#define CYGDAT_DEVS_ADC_CORTEXM_STM32_ADC1_CHANNEL14_NAME "/dev/adc014"
#define CYGDAT_DEVS_ADC_CORTEXM_STM32_ADC1_CHANNEL14_BUFSIZE    128
STM32_ADC_CHANNEL(1, 14)

#define CYGDAT_DEVS_ADC_CORTEXM_STM32_ADC1_CHANNEL16_NAME "/dev/adc016"
#define CYGDAT_DEVS_ADC_CORTEXM_STM32_ADC1_CHANNEL16_BUFSIZE    128
STM32_ADC_CHANNEL(1, 16)
// Added end by reille 2013.04.24



==============================================================================

[eCos] Ver0.3: 2013.04.21
-------------------------
在Ver0.2版本的基础上，添加了CYGPKG_ID_ADC(Generic ADC support)包，同时，启用了
Hardware ADC device driver和ADC1配置项，并打开了ADC1的ADC channel14和ADC channel16，
其它ADC配置采用默认配置。


==============================================================================

[eCos] Ver0.2: 2013.04.14
-------------------------
在Ver0.1版本的基础上，仅使能了配置项：CYGPKG_IO_SERIAL_DEVICES，与该配置项相关
的子配置项采用默认值。


==============================================================================

[redboot] Ver0.1: 2013.04.13
----------------------------
原始版本，直接使用eCos源码包Template(Hardware：ST STM32F10XXX platform board，
Packages：redboot)生成的。注：ST STM32F10XXX platform board是基于ST STM3210E
EVAL board创建的。


==============================================================================

[eCos] Ver0.1: 2013.04.06
-------------------------
原始版本，直接使用eCos源码包Template(Hardware：ST STM32F10XXX platform board，
Packages：default)生成的。注：ST STM32F10XXX platform board是基于ST STM3210E
EVAL board创建的。




//----------------------------------------------------------------------------
// End of file