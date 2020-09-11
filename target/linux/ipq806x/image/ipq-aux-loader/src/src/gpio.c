/*
 * Auxiliary kernel loader for Qualcom IPQ-4XXX/806X based boards
 *
 * Copyright (C) 2020 Sergey Sergeev <adron@mstnt.com>
 *
 * Some structures and code has been taken from the U-Boot project.
 *	(C) Copyright 2008 Semihalf
 *	(C) Copyright 2000-2005
 *	Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <gpio.h>
#include <iomap.h>
#include <io.h>
#include <types.h>

/*******************************************************
Function description: configure GPIO functinality
Arguments :
unsigned int gpio - Gpio number
unsigned int func - Functionality number
unsigned int dir  - direction 0- i/p, 1- o/p
unsigned int pull - pull up/down, no pull range(0-3)
unsigned int drvstr - range (0 - 7)-> (2- 16)MA steps of 2
unsigned int oe - 0 - Disable, 1- Enable.

Return : None
*******************************************************/
void gpio_tlmm_config(unsigned int gpio, unsigned int func,
                      unsigned int out, unsigned int pull,
                      unsigned int drvstr, unsigned int oe){
	unsigned int val = 0;
	val |= pull;
	val |= func << 2;
	val |= drvstr << 6;
	val |= oe << 9;
	unsigned int *addr = (unsigned int *)GPIO_CONFIG_ADDR(gpio);
	writel(val, addr);

	/* Output value is only relevant if GPIO has been configured for fixed
	 * output setting - i.e. func == 0 */
	if (func == 0){
		addr = (unsigned int *)GPIO_IN_OUT_ADDR(gpio);
		val = readl(addr);
		val |= out << 1;
		writel(val, addr);
	}
	return;
}

void gpio_set_value(unsigned int gpio, unsigned int out){
	unsigned int *addr = (unsigned int *)GPIO_IN_OUT_ADDR(gpio);
	unsigned int val = 0;

	val = readl(addr);
	val &= ~(0x2);
	val |= out << 1;
	writel(val, addr);
}

unsigned int gpio_get_value(unsigned int gpio){
	unsigned int *addr = (unsigned int *)GPIO_IN_OUT_ADDR(gpio);
  uint32_t val = readl(addr);
  val &= (~(1 << 1));
	return val;
}
