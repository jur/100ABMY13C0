/*
 * Realtek Semiconductor Corp.
 *
 * bsp/usb.c:
 *     bsp I2C initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include "bspchip.h"


#define rtlRegRead(addr)        \
        (*(volatile u32 *)(addr))

#define rtlRegWrite(addr, val)  \
        ((*(volatile u32 *)(addr)) = (val))

static inline u32 rtlRegMask(u32 addr, u32 mask, u32 value)
{
        u32 reg;

        reg = rtlRegRead(addr);
        reg &= ~mask;
        reg |= value & mask;
        rtlRegWrite(addr, reg);
        reg = rtlRegRead(addr); /* flush write to the hardware */

        return reg;
}

/* i2c Host Controller */

static struct resource bsp_i2c_resource[] = {
	[0] = {
		.start = BSP_I2C_MAPBASE,
		.end = BSP_I2C_MAPBASE + BSP_I2C_MAPSIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = BSP_PS_I2C_IRQ,
		.end = BSP_PS_I2C_IRQ,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 bsp_i2c_dmamask = 0xFFFFFFFFUL;

struct platform_device bsp_i2c_device = {
	.name = "rtl-i2c",
	.id = -1,
	.num_resources = ARRAY_SIZE(bsp_i2c_resource),
	.resource = bsp_i2c_resource,
	.dev = {
		.dma_mask = &bsp_i2c_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

static struct platform_device *bsp_i2c_devs[] __initdata = {	&bsp_i2c_device,  };

static int __init bsp_i2c_init(void)
{
	int ret;

	printk("INFO: initializing i2c devices ...\n");

	/* PINMUX setting */
	#define RTL_GPIO_MUX4		0xB800010C
	#define RTL_GPIO_DATA_P5GTXC	(6<<3)
	#define RTL_GPIO_DATA_P5TXD4	(8<<17)
	#define RTL_GPIO_MUX4_DATA	(RTL_GPIO_DATA_P5GTXC| RTL_GPIO_DATA_P5TXD4)
	rtlRegMask(RTL_GPIO_MUX4, 0xF<<3 | 0xF<<17, RTL_GPIO_MUX4_DATA);

	ret = platform_add_devices(bsp_i2c_devs, ARRAY_SIZE(bsp_i2c_devs));
	if (ret < 0) {
		printk("ERROR: unable to add devices\n");
		return ret;
	}

	return 0;
}
arch_initcall(bsp_i2c_init);
