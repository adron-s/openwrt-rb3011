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

#include <iomap.h>
#include <io.h>
#include <types.h>

#if defined CONFIG_IPQ806X

#define GPT_FREQ_KHZ   32
#define GPT_FREQ			 (GPT_FREQ_KHZ * 1000)	/* 32 KHz */
#define GPT_ENABLE_EN  1

/**
 * timer_init - initialize timer
 */
int timer_init(void){
	writel(0, GPT_ENABLE);
	writel(GPT_ENABLE_EN, GPT_ENABLE);
	return 0;
}

/**
 * __udelay -  generates micro second delay.
 * @usec: delay duration in microseconds
 *
 * With 32KHz clock, minimum possible delay is 31.25 Micro seconds and
 * its multiples. In Rumi GPT clock is 32 KHz
 */
void __udelay(unsigned long usec){
	unsigned int val;
	unsigned long now, last;
	unsigned long runcount;

	usec = (usec + GPT_FREQ_KHZ - 1) / GPT_FREQ_KHZ;
	last = readl(GPT_COUNT_VAL);
	runcount = last;
	val = usec + last;

	do {
		now = readl(GPT_COUNT_VAL);
		if (last > now)
			runcount += ((GPT_FREQ - last) + now);
		else
			runcount += (now - last);
		last = now;
	} while (runcount < val);
}

void udelay(unsigned long usec){
	__udelay(usec);
}

void mdelay(unsigned long msec){
	while (msec--)
		udelay(1000);
}

#endif /* CONFIG_IPQ806X */
