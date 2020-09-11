/*
 * Auxiliary kernel loader for Qualcom IPQ-4XXX/806X based boards
 *
 * Copyright (C) 2019 Sergey Sergeev <adron@mstnt.com>
 *
 * Some parts of this code was based on the OpenWrt specific lzma-loader
 * for the Atheros AR7XXX/AR9XXX based boards:
 *	Copyright (C) 2011 Gabor Juhos <juhosg@openwrt.org>
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

#include <printf.h>
#include <iomap.h>
#include <io.h>
#include <types.h>
#include <uimage/legacy.h>

#define IPQ_ARCH 4200

/* beyond the image end, size not known in advance */
extern unsigned char workspace[];
u32 arch = 0;
void (*kernel_entry)(int zero, int arch, unsigned int params);
u32 kernel_p0 = 0;
u32 kernel_p1 = IPQ_ARCH;
u32 kernel_p2 = NULL;

/* base stack poinet - 16 mb. stack grows left!
	 boot_params(12b)<--16M of stack--TEXT_BASE,_start--END of program
	 !!! TODO: replace to like workspace static mem region !!!
*/
//u32 *boot_params = (void*)(CONFIG_SYS_TEXT_BASE - 0x100000C);

void bi_dram_0_set_ranges(u32, u32);
int cleanup_before_linux(void);
void enable_caches(void);
extern u32 owl_get_sp(void);
void serial_putc(char c);
void watchdog_setup(int);
unsigned long int ntohl(unsigned long int);
void dump_mem(unsigned char *, char *);
void reset_cpu(ulong);
int fdt_check_header(void *, u32);
char *fdt_get_prop(void *, char *, char *, u32 *);
int lzma_gogogo(void *, void *, u32, u32 *);
void ipq806x_rb3011_preinits(void);

void my_memcpy(void *dst, const void *src, u32 n){
	const void *end = src + n;
	for(; src + 4 <= end; src += 4, dst += 4)
		*((u32*)dst) = *((u32*)src);
	for(; src < end; src++, dst++)
		*((u8*)dst) = *((u8*)src);
}

int handle_legacy_header(void *_kernel_data_start, u32 kern_image_len){
	legacy_image_header_t *image = (void*)_kernel_data_start;
	void *kernel_load = (void*)ntohl(image->ih_load);
	void *kernel_ep = (void*)ntohl(image->ih_ep);
	void *src = (void*)_kernel_data_start + sizeof(legacy_image_header_t);
	void *dst = kernel_load;
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	u32 kernel_body_len = ntohl(image->ih_size);

	debug("  size = %u\n", kernel_body_len);
	debug("  name = '%s'\n", (char*)image->ih_name);
	debug("  load = 0x%08x\n", kernel_load);
	debug("  ep = 0x%08x\n", kernel_ep);

	//check ih_size for adequate value
	if(kern_image_len - sizeof(legacy_image_header_t) < kernel_body_len){
		printf("\n");
		printf("Error ! Kernel sizes mismath detected !\n");
		printf("  details: %d - %d < %d !\n", kern_image_len,
			sizeof(legacy_image_header_t), kernel_body_len);
		return -99;
	}
	debug("\n");
	printf("Copy kernel...");
	my_memcpy(dst, src, kernel_body_len);
	printf("Done\n");
	kernel_entry = kernel_ep;
	return 0;
}

/* FIT - Flattened Image Tree.
	 ! Not to be confused with an FDT - Flattened Device Tree !
	 ! MAGIC of FIT == ~MAGIC of FDT !
*/
#define FIT_KERNEL_NODE_NAME "kernel@1"
#define FIT_DTB_NODE_NAME "fdt@1"
int handle_fit_header(void *_kernel_data_start, u32 kern_image_len){
	void *data = (void*)_kernel_data_start;
	void *kernel_load = NULL;
	void *kernel_ep = NULL;
	char *kernel_name = NULL;
	char *kernel_compr = NULL;
	void *dtb_data = NULL; //device tree blob
	void *dtb_dst = (void*)workspace;
	u32 dtb_body_len = 0;
	u32 kernel_body_len = 0;
	u32 kernel_uncompr_size = 0;
	void *src = NULL;
	void *dst = NULL;
	char *tmp_c;
	int ret;
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");
	/* Do FDT header base checks */
	ret = fdt_check_header(data, kern_image_len);
	if(ret){
		printf("Error ! FDT header is corrupted! check_ret = %d\n", ret);
		return -99;
	}
	if(!(kernel_name = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "description", NULL)))
		return -98;
	if(!(tmp_c = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "load", NULL)))
		return -97;
	kernel_load = (void*)ntohl(*(u32*)tmp_c);
	dst = kernel_load;
	if(!(tmp_c = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "entry", NULL)))
		return -96;
	kernel_ep = (void*)ntohl(*(u32*)tmp_c);
	if(!(src = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "data", &kernel_body_len)))
		return -95;
	if(!(kernel_compr = fdt_get_prop(data, FIT_KERNEL_NODE_NAME, "compression", NULL)))
		return -94;
	if(!(dtb_data = fdt_get_prop(data, FIT_DTB_NODE_NAME, "data", &dtb_body_len)))
		return -93;

	debug("  size = %u\n", kernel_body_len);
	debug("  name = '%s'\n", kernel_name);
	debug("  load = 0x%08x\n", kernel_load);
	debug("  ep = 0x%08x\n", kernel_ep);
	debug("  compr = %s\n", kernel_compr);

	//check kernel@1->data size for adequate value
	if(kernel_body_len >= kern_image_len){
		printf("\n");
		printf("Error ! Kernel sizes mismath detected !\n");
		printf("  details: %d >= %d !\n", kernel_body_len, kern_image_len);
		return -99;
	}
	debug("\n");
	printf("Extracting LZMA kernel...");
	watchdog_setup(30);
	/* without this all cpu operations is very very slow ! */
	if(dst < src){
		/* setup D-Cache ranges. our loader head + 32M */
		bi_dram_0_set_ranges((u32)dst, (u32)(src - dst + (void*)0x2000000));
		enable_caches(); /* Enable I and D caches ONLY for LZMA op */
		lzma_gogogo(dst, src, kernel_body_len, &kernel_uncompr_size);
	}
	cleanup_before_linux(); /* Disable I and D caches */

	/* prepare device tree blob data for copy after kernel */
	my_memcpy(dtb_dst, dtb_data, dtb_body_len);
	//dump_mem(workspace, "dtb_data:");

	kernel_entry = kernel_ep;
	kernel_p2 = (u32)dtb_dst;
	return 0;
}

void loader_main(u32 head_text_base, u32 _arch)
{
	extern char _kernel_data_start[];
	extern char _kernel_data_end[];
	u32 kern_image_len = _kernel_data_end  - _kernel_data_start;
	uint32_t *_magic = (void*)_kernel_data_start;
	u32 magic;
	int ret = -100;

	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	printf("\n");
	printf("OpenWrt kernel loader for Qualcomm IPQ-4XXX/IPQ-806X\n");
	printf("Copyright 2019-2020 Sergey Sergeev <adron@mstnt.com>\n");

#ifdef RB3011_PREINITS
	printf("\n");
	ipq806x_rb3011_preinits();
#endif /* RB3011_PREINITS */

	debug("\n");
	debug("  head loader TEXT_BASE = 0x%08X\n", head_text_base);
	debug("kernel loader TEXT_BASE = 0x%08X\n", CONFIG_SYS_TEXT_BASE);
	//printf("kernel_data_start = 0x%08X\n", _kernel_data_start);
	//printf("kernel_data_end = 0x%08X\n", _kernel_data_end);
	printf("\n");

	//watchdog_setup(5); for(;;);
	//reset_cpu(0);

	arch = _arch;
	kernel_p1 = arch;
	debug("ARCH = %d\n", arch);
	if(arch != IPQ_ARCH){
		printf("Critical alert ! ARCH mismatch: %u vs %u\n", arch, IPQ_ARCH);
		watchdog_setup(5); for(;;);
	}
	magic = ntohl(*_magic);
	debug("Kernel image header:\n");
	switch(magic){
		case LEGACY_IH_MAGIC:
			debug("  magic = 0x%x, Legacy uImage\n", magic);
			ret = handle_legacy_header(_kernel_data_start, kern_image_len);
			break;
		case FIT_IH_MAGIC:
			debug("  magic = 0x%x, FIT uImage\n", magic);
			ret = handle_fit_header(_kernel_data_start, kern_image_len);
			break;
		default:
			printf("  magic = 0x%x, UNKNOWN !!!\n", magic);
	}
	if(ret){
		printf("\n");
		printf("Op ret = %d\n", ret);
		printf("Auto reboot in 5 sec\n");
		watchdog_setup(5); for(;;);
	}

	printf("Starting kernel at 0x%08x\n", kernel_entry);
	printf("\n");
	cleanup_before_linux();
	kernel_entry(kernel_p0, kernel_p1, kernel_p2);
	reset_cpu(0);
}

#ifdef RB3011_PREINITS
#include <gpio.h>
#include <timer.h>
#include <mdio.h>

#define GPIO_MDIO1			10
#define GPIO_MDC1				11
#define GPIO_SW0_RESET	16
#define GPIO_SW1_RESET	17
#define	GPIO_USER_LED		33
#define GPIO_LCD_POWER	48
#define	GPIO_RST_BTN		66

void do_phys_pwdown(int mdio_num){
	int a;
	mdio_reg_base = NSS_GMAC0_BASE;
	if(mdio_num == 1)
		mdio_reg_base = NSS_GMAC1_BASE;
	for(a = 0; a <= 4; a++){
		int val, limiter = 0;
		while(limiter++ < 100){
			val = ipq_phy_read(a, MII_PHYSID1);
			//waiting for phy ready state
			if(val == 0x4d){
				val = ipq_phy_read(a, MII_BMCR);
				val |= BMCR_PDOWN;
				ipq_phy_write(a, MII_BMCR, val);
				//printf("Phy %d:%d is PDOWN\n", mdio_num, a);
				break;
			}
			udelay(1000);
		}
		if(limiter > 100){
			printf("Phy %d:%d is NOT READY !!!\n", mdio_num, a);
			gpio_set_value(GPIO_LCD_POWER, 1); //enable LCD power if error
		}
		//printf("phy %d: reg 0x00: 0x%x\n", a, ipq_phy_read(a, 0x00));
		//printf("phy %d: reg 0x02: 0x%x\n", a, ipq_phy_read(a, 0x02));
		//printf("phy %d: reg 0x03: 0x%x\n", a, ipq_phy_read(a, 0x03));
	}
}

void ipq806x_rb3011_preinits(){
	printf("Doing preinits for Mikrotik RB3011\n");
	//timer init(it needs for udelay)
	timer_init();
	//configuring gpio
	gpio_tlmm_config(GPIO_LCD_POWER, 0, GPIO_OUT_LOW, GPIO_PULL_DOWN, GPIO_16MA, GPIO_OE_ENABLE); //out - LCD
	gpio_tlmm_config(GPIO_USER_LED, 0, GPIO_OUT_LOW, GPIO_PULL_DOWN, GPIO_16MA, GPIO_OE_ENABLE); //out - LED
	//gpio_tlmm_config(GPIO_RST_BTN, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA, GPIO_OE_DISABLE);  //in - reset button
	gpio_tlmm_config(GPIO_MDIO1, 5, GPIO_INPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_OE_DISABLE); //in - func5 - mdio
	gpio_tlmm_config(GPIO_MDC1, 6, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA, GPIO_OE_ENABLE);  //out - func6 - mdio
	gpio_tlmm_config(GPIO_SW0_RESET, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA, GPIO_OE_ENABLE); //out - func0 - gpio - switch0 reset
	gpio_tlmm_config(GPIO_SW1_RESET, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_16MA, GPIO_OE_ENABLE); //out - func0 - gpio - switch1 reset
	gpio_set_value(GPIO_LCD_POWER, 0); //disable LCD power
  //disable switch0, phy_transivers0, switch1, phy_transivers1
	gpio_set_value(GPIO_SW0_RESET, 0);
	gpio_set_value(GPIO_SW1_RESET, 0);
  mdelay(20);
  //enable switch0, phy_transivers0, switch1, phy_transivers1
  gpio_set_value(GPIO_SW0_RESET, 1);
	gpio_set_value(GPIO_SW1_RESET, 1);
  mdelay(20);
  //set all phy transivers to power down state
  do_phys_pwdown(0);
	do_phys_pwdown(1);
	//debug
	if(0){
		int a = 0, val = GPIO_OUT_HIGH;
		watchdog_setup(10);
		while(1){
			mdelay(1000);
			if(val == GPIO_OUT_LOW)
				val = GPIO_OUT_HIGH;
			else
				val = GPIO_OUT_LOW;
			gpio_set_value(GPIO_USER_LED, val);
			printf("Hello...%d\n", a++);
			//printf("reset button value = 0x%x\n", gpio_get_value(66));
		}
	}
}
#endif /* RB3011_PREINITS */
