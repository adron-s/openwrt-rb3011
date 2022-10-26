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
#include <timer.h>
#include <mdio.h>

/* MII address register definitions. from UBOOT/../nss/ipq_mdio.h */
#define MII_BUSY				(1 << 0)
#define MII_WRITE				(1 << 1)
#define MII_CLKRANGE_60_100M			(0)
#define MII_CLKRANGE_100_150M			(0x4)
#define MII_CLKRANGE_20_35M			(0x8)
#define MII_CLKRANGE_35_60M			(0xC)
#define MII_CLKRANGE_150_250M			(0x10)
#define MII_CLKRANGE_250_300M			(0x14)
#define MII_DATA_REG_ADDR			(0x14)
#define MII_ADDR_REG_ADDR			(0x10)
#define MII_MDIO_TIMEOUT			(10000)
#define MIIADDRSHIFT				(11)
#define MIIREGSHIFT				(6)
#define MII_REGMSK	 			(0x1F << 6)
#define MII_ADDRMSK				(0x1F << 11)
#define MII_PHY_STAT_SHIFT			(10)
#define AUTO_NEG_ENABLE				(1 << 12)

#if defined CONFIG_IPQ806X

unsigned int mdio_reg_base = NSS_GMAC0_BASE;

unsigned int ipq_mdio_read(unsigned int phy_addr, unsigned int reg_offset, unsigned short *data){
	unsigned int reg_base = mdio_reg_base;
	unsigned int miiaddr;
	unsigned int ret_val;
	unsigned int limiter = 0;

	miiaddr = (((phy_addr << MIIADDRSHIFT) & MII_ADDRMSK) |
	((reg_offset << MIIREGSHIFT) & MII_REGMSK));

	miiaddr |= (MII_BUSY | MII_CLKRANGE_250_300M);
	writel(miiaddr, (reg_base + MII_ADDR_REG_ADDR));
	udelay(10);

	while(limiter++ < 10){
		if (!(readl(reg_base + MII_ADDR_REG_ADDR) & MII_BUSY)) {
			ret_val = readl(reg_base + MII_DATA_REG_ADDR);
			if (data != NULL)
				*data = ret_val;
			return ret_val;
		}
		udelay(1000);
		//printf("%s: limiter = %d\n", __func__, limiter);
	}
	return -1;
}

unsigned int ipq_mdio_write(unsigned int phy_addr, unsigned int reg_offset, unsigned short data){
	unsigned int reg_base = mdio_reg_base;
	unsigned int miiaddr;
	unsigned int limiter = 0;

	writel(data, (reg_base + MII_DATA_REG_ADDR));

	miiaddr = (((phy_addr << MIIADDRSHIFT) & MII_ADDRMSK) |
			((reg_offset << MIIREGSHIFT) & MII_REGMSK) |
			(MII_WRITE));

	miiaddr |= (MII_BUSY | MII_CLKRANGE_250_300M);
	writel(miiaddr, (reg_base + MII_ADDR_REG_ADDR));
	udelay(10);

	while(limiter++ < 10){
		if (!(readl(reg_base + MII_ADDR_REG_ADDR) & MII_BUSY)) {
			return 0;
		}
		udelay(1000);
	}
	return -1;
}

int ipq_phy_read(int addr, int regnum){
	return ipq_mdio_read(addr, regnum, NULL);
}

int ipq_phy_write(int addr, int regnum, unsigned short value){
	return ipq_mdio_write(addr, regnum, value);
}

#endif /* CONFIG_IPQ806X */
