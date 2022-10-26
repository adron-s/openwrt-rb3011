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

#ifndef __IPQ_MDIO_H
#define __IPQ_MDIO_H

#define MII_BMCR        0x00    /* Basic mode control register */
#define MII_BMSR        0x01    /* Basic mode status register  */
#define MII_PHYSID1     0x02    /* PHYS ID 1                   */
#define MII_PHYSID2     0x03    /* PHYS ID 2                   */

#define BMCR_PDOWN      0x0800  /* Powerdown */
#define BMCR_RESET      0x8000  /* Reset */

#if defined CONFIG_IPQ806X

#define NSS_GMAC0_BASE	0x37000000
#define NSS_GMAC1_BASE	0x37200000

extern unsigned int mdio_reg_base;

unsigned int ipq_mdio_read(unsigned int, unsigned int, unsigned short *);
unsigned int ipq_mdio_write(unsigned int, unsigned int, unsigned short);
int ipq_phy_read(int, int);
int ipq_phy_write(int, int, unsigned short);

#endif /* CONFIG_IPQ806X */

#endif /* __IPQ_MDIO_H */
