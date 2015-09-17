/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk-private.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/clk/sunxi.h>
#include <mach/sys_config.h>
#include <mach/sunxi-smc.h>
#include "clk-sunxi.h"
#include "clk-factors.h"
#include "clk-periph.h"
#include "clk-sun8iw6.h"
#include "clk-sun8iw6_tbl.c"

#ifndef CONFIG_EVB_PLATFORM
    #define LOCKBIT(x) 31
#else
    #define LOCKBIT(x) x
#endif
static DEFINE_SPINLOCK(clk_lock);
void __iomem *sunxi_clk_base;
void __iomem *sunxi_clk_cpus_base=0;
int	sunxi_clk_maxreg =SUNXI_CLK_MAX_REG;
int cpus_clk_maxreg = 0;
#ifdef CONFIG_SUNXI_CLK_DUMMY_DEBUG
unsigned int dummy_reg[1024]; 
unsigned int dummy_readl(unsigned int* regaddr)
{
	unsigned int val;
	val = *regaddr;
	printk("--%s-- dummy_readl to read reg 0x%x with val 0x%x\n",__func__,((unsigned int)regaddr - (unsigned int)&dummy_reg[0]),val);
	return val;
}
void  dummy_writel(unsigned int val,unsigned int* regaddr)
	{
		*regaddr = val;
		printk("--%s-- dummy_writel to write reg 0x%x with val 0x%x\n",__func__,((unsigned int)regaddr - (unsigned int)&dummy_reg[0]),val);
	}

void dummy_reg_init(void)
{
	memset(dummy_reg,0x0,sizeof(dummy_reg));				
	dummy_reg[PLL1_CFG/4]=0x00001000;
	dummy_reg[PLL2_CFG/4]=0x00035514;
	dummy_reg[PLL3_CFG/4]=0x03006207;
	dummy_reg[PLL4_CFG/4]=0x03006207;
	dummy_reg[PLL5_CFG/4]=0x00001000;
	dummy_reg[PLL6_CFG/4]=0x00041811;
	dummy_reg[PLL8_CFG/4]=0x03006207;
	dummy_reg[PLL9_CFG/4]=0x03001300;
	dummy_reg[PLL10_CFG/4]=0x03006207;
	dummy_reg[CPU_CFG/4]=0x00001000;
	dummy_reg[AHB1_CFG/4]=0x00001010;
	dummy_reg[APB2_CFG/4]=0x01000000;
	dummy_reg[ATS_CFG/4]=0x80000000;
	dummy_reg[PLL_LOCK/4]=0x000000FF;
	dummy_reg[CPU_LOCK/4]=0x000000FF;
}
#endif // of CONFIG_SUNXI_CLK_DUMMY_DEBUG

/*                          ns  nw  ks  kw  ms  mw  ps  pw  d1s  d1w  d2s  d2w  {frac  out  mode}  en-s   sdmss  sdmsw  sdmpat      sdmval*/
SUNXI_CLK_FACTORS(pll_cpu,   8,  8,  0,  0,  0,  0, 16,  1,  0,   0,    0,  0,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS(pll_audio, 8,  8,  0,  0,  0,  0,  0,  6,  16,  1,   18,  1,    0,    0,   0,     31,   24,     1,       PLL_AUDIOPAT, 0xc000e147);
SUNXI_CLK_FACTORS(pll_video0,8,  8,  0,  0,  0,  0,  0,  2,  16,  1,    0,  0,    0,    0,   0,     31,   24,     0,       PLL_VIDEO0PAT,0xd1303333);
SUNXI_CLK_FACTORS(pll_media, 8,  8,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0);
SUNXI_CLK_FACTORS_UPDATE(pll_ddr,   8,  6,  0,  0,  0,  0,  0,  0,  16,  1,   18,  1,    0,    0,   0,     31,    0,     0,       0,        0 , 30);
SUNXI_CLK_FACTORS(pll_video1,8,  8,  0,  0,  0,  0,  0,  2,  16,  1,    0,  0,    0,    0,   0,     31,    0,     0,       0,        0);

static int get_factors_pll_cpu0(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
   int index;
    u64 tmp_rate;
    if(!factor)
        return -1;
    tmp_rate = rate>pllcpu0_max ? pllcpu0_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
    if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_cpu,factor, factor_pllcpu0_tbl,index,sizeof(factor_pllcpu0_tbl)/sizeof(struct sunxi_clk_factor_freq)))
        return -1;
    return 0;
}
static int get_factors_pll_cpu1(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
   int index;
    u64 tmp_rate;
    if(!factor)
        return -1;
    tmp_rate = rate>pllcpu1_max ? pllcpu1_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
    if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_cpu,factor, factor_pllcpu1_tbl,index,sizeof(factor_pllcpu1_tbl)/sizeof(struct sunxi_clk_factor_freq)))
        return -1;
    return 0;
}
static int get_factors_pll_audio(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    if(rate == 22579200) {
        factor->factorn = 54;
        factor->factorp = 28;
        factor->factord1 = 0;
        factor->factord2 = 1;
        sunxi_clk_factor_pll_audio.sdmval = 0xc00121ff;
    } else if(rate == 24576000) {
        factor->factorn = 61;
        factor->factorp = 29;
        factor->factord1 = 0;
        factor->factord2 = 1;
        sunxi_clk_factor_pll_audio.sdmval = 0xc000e147;
    } else
        return -1;
    return 0;
}
static int get_factors_pll_video0(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate;
    int index;
    if(!factor)
        return -1;

    tmp_rate = rate>pllvideo_max ? pllvideo_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
 	if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_video0,factor, factor_pllvideo_tbl,index,sizeof(factor_pllvideo_tbl)/sizeof(struct sunxi_clk_factor_freq)))
 		return -1;
    return 0;
}
static int get_factors_pll_video1(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate;
    int index;
    if(!factor)
        return -1;

    tmp_rate = rate>pllvideo_max ? pllvideo_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
 	if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_video1,factor, factor_pllvideo_tbl,index,sizeof(factor_pllvideo_tbl)/sizeof(struct sunxi_clk_factor_freq)))
 		return -1;
    return 0;
}
static int get_factors_pll_ve(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate;
    int index;
    if(!factor)
        return -1;

    tmp_rate = rate>pllve_max ? pllve_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
 		if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_media,factor, factor_pllve_tbl,index,sizeof(factor_pllve_tbl)/sizeof(struct sunxi_clk_factor_freq)))
 			return -1;
    return 0;
}

static int get_factors_pll_ddr(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
	int index;
	u64 tmp_rate;
	if (!factor)
		return -1;
	tmp_rate = rate > pllddr_max ? pllddr_max : rate;
	do_div(tmp_rate, 1000000);
	index = tmp_rate;
	if (sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_ddr, factor,
					 factor_pllddr_tbl, index,
					 sizeof(factor_pllddr_tbl)/sizeof(struct sunxi_clk_factor_freq)))
		return -1;

	return 0;
}
static int get_factors_pll_periph(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
   int index;
    u64 tmp_rate;
    if(!factor)
        return -1;
    tmp_rate = rate>pllperiph_max ? pllperiph_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
        if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_media,factor, factor_pllperiph_tbl,index,sizeof(factor_pllperiph_tbl)/sizeof(struct sunxi_clk_factor_freq)))
 			return -1;
    return 0;
}
static int get_factors_pll_gpu(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate;
    int index;
    if(!factor)
        return -1;

    tmp_rate = rate>pllgpu_max ? pllgpu_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
 		if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_media,factor, factor_pllgpu_tbl,index,sizeof(factor_pllgpu_tbl)/sizeof(struct sunxi_clk_factor_freq)))
 			return -1;
    return 0;
}
static int get_factors_pll_hsic(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate;
    int index;
    if(!factor)
        return -1;

    tmp_rate = rate>pllhsic_max ? pllhsic_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
 		if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_media,factor, factor_pllhsic_tbl,index,sizeof(factor_pllhsic_tbl)/sizeof(struct sunxi_clk_factor_freq)))
 			return -1;
    return 0;
}
static int get_factors_pll_de(u32 rate, u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate;	
    int index;    
    if(!factor)
        return -1;

    tmp_rate = rate>pllde_max ? pllde_max : rate;
    do_div(tmp_rate, 1000000);
    index = tmp_rate;
 		if(sunxi_clk_get_common_factors_search(&sunxi_clk_factor_pll_media,factor, factor_pllde_tbl,index,sizeof(factor_pllde_tbl)/sizeof(struct sunxi_clk_factor_freq)))
 			return -1;
    return 0;
}

static unsigned long calc_rate_pll_cpu(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn);
    do_div(tmp_rate, (factor->factorp?4:1));
    return (unsigned long)tmp_rate;
}

static unsigned long calc_rate_pll_audio(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    if((factor->factorn == 54)  && (factor->factord1 == 0) && (factor->factord2 == 1) && (factor->factorp == 28))
        return 22579200;
    else if((factor->factorn == 61) && (factor->factord1 == 0) && (factor->factord2 == 1) && (factor->factorp == 29))
        return 24576000;
    else
    {
        tmp_rate = tmp_rate * (factor->factorn);
        do_div(tmp_rate, (factor->factord1+1) * (factor->factord2+1) * (factor->factorp+1));
        return (unsigned long)tmp_rate;
    }
}
static unsigned long calc_rate_video(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn);
    do_div(tmp_rate, (factor->factord1+1) *(1 << factor->factorp));
    return (unsigned long)tmp_rate;
}
static unsigned long calc_rate_ddr(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn+1);
    do_div(tmp_rate, (factor->factord1+1) *(factor->factord2+1));
    return (unsigned long)tmp_rate;
}
static unsigned long calc_rate_media(u32 parent_rate, struct clk_factors_value *factor)
{
    u64 tmp_rate = (parent_rate?parent_rate:24000000);
    tmp_rate = tmp_rate * (factor->factorn);
    do_div(tmp_rate, (factor->factord1+1) *(factor->factord2+1));
    return (unsigned long)tmp_rate;
}


u8 get_parent_mipi(struct clk_hw *hw)
{
    u8 parent;
    unsigned long reg;
	struct sunxi_clk_factors *factor = to_clk_factor(hw);

    if(!factor->reg)
        return 0;
    reg = readl(factor->reg);
    parent = GET_BITS(21, 1, reg);

    return parent;
}


int set_parent_mipi(struct clk_hw *hw, u8 index)
{
    unsigned long reg;
	struct sunxi_clk_factors *factor = to_clk_factor(hw);

    if(!factor->reg)
        return 0;
    reg = readl(factor->reg);
    reg = SET_BITS(21, 1, reg, index);
    writel(reg, factor->reg);
    return 0;
}
static const char *hosc_parents[] = {"hosc"};

struct clk_ops mipi_ops;

struct factor_init_data sunxi_factos[] = {
    /* name         parent        parent_num, flags       reg       lock_reg   lock_bit    config                     get_factors          calc_rate       priv_ops*/
    {"pll_cpu0",    hosc_parents, 1,CLK_GET_RATE_NOCACHE, PLL_CPU0,  LOCK_STAT,   LOCKBIT(0),&sunxi_clk_factor_pll_cpu,    &get_factors_pll_cpu0,   &calc_rate_pll_cpu   ,(struct clk_ops*)NULL},
    {"pll_cpu1",    hosc_parents, 1,CLK_GET_RATE_NOCACHE, PLL_CPU1,  LOCK_STAT,   LOCKBIT(1),&sunxi_clk_factor_pll_cpu,    &get_factors_pll_cpu1,   &calc_rate_pll_cpu   ,(struct clk_ops*)NULL},
    {"pll_audio",   hosc_parents, 1,          0,          PLL_AUDIO, LOCK_STAT,   LOCKBIT(2),&sunxi_clk_factor_pll_audio,  &get_factors_pll_audio,  &calc_rate_pll_audio ,(struct clk_ops*)NULL},
    {"pll_video0",  hosc_parents, 1,          0,          PLL_VIDEO0,LOCK_STAT,   LOCKBIT(3),&sunxi_clk_factor_pll_video0, &get_factors_pll_video0, &calc_rate_video     ,(struct clk_ops*)NULL},
    {"pll_ve",      hosc_parents, 1,          0,          PLL_VE,    LOCK_STAT,   LOCKBIT(4),&sunxi_clk_factor_pll_media,  &get_factors_pll_ve,     &calc_rate_media  ,(struct clk_ops*)NULL},
    {"pll_ddr",     hosc_parents, 1,CLK_GET_RATE_NOCACHE, PLL_DDR,   LOCK_STAT,   LOCKBIT(5),&sunxi_clk_factor_pll_ddr,    &get_factors_pll_ddr,    &calc_rate_ddr       ,(struct clk_ops*)NULL},
    {"pll_periph",  hosc_parents, 1,          0,          PLL_PERIPH,LOCK_STAT,   LOCKBIT(6),&sunxi_clk_factor_pll_media,  &get_factors_pll_periph, &calc_rate_media  ,(struct clk_ops*)NULL},
    {"pll_gpu",     hosc_parents, 1,          0,          PLL_GPU,   LOCK_STAT,   LOCKBIT(7),&sunxi_clk_factor_pll_media,  &get_factors_pll_gpu,    &calc_rate_media  ,(struct clk_ops*)NULL},
    {"pll_hsic",    hosc_parents, 1,          0,          PLL_HSIC,  LOCK_STAT,   LOCKBIT(8),&sunxi_clk_factor_pll_media,  &get_factors_pll_hsic,   &calc_rate_media  ,(struct clk_ops*)NULL},
    {"pll_de",      hosc_parents, 1,          0,          PLL_DE,    LOCK_STAT,   LOCKBIT(9),&sunxi_clk_factor_pll_media,  &get_factors_pll_de,     &calc_rate_media  ,(struct clk_ops*)NULL},
    {"pll_video1",  hosc_parents, 1,          0,          PLL_VIDEO1,LOCK_STAT,   LOCKBIT(10),&sunxi_clk_factor_pll_video1,&get_factors_pll_video1, &calc_rate_video     ,(struct clk_ops*)NULL},

};


struct periph_init_data {
    const char          *name;
    unsigned long       flags;        
    const char          **parent_names;
    int                 num_parents;
    struct sunxi_clk_periph *periph;
};

static const char *cluster0_parents[]   = {"hosc", "pll_cpu0"};
static const char *cluster1_parents[]   = {"hosc", "pll_cpu1"};
static const char *axi0_parents[]   = {"cluster0"};
static const char *axi1_parents[]   = {"cluster1"};
static const char *pll_periphahb1_parents[] = {"pll_periph"};
static const char *ahb1_parents[] = {"losc", "hosc", "pll_periphahb1", "pll_periphahb1"};
static const char *apb1_parents[] = {"ahb1"};
static const char *apb2_parents[] = {"losc", "hosc", "pll_periph", "pll_periph"};
static const char *cci400_parents[]    = {"hosc","pll_periph","pll_hsic",""}; //cci400
static const char *perph1_parents[] = {"hosc", "pll_periph","",""};
static const char *audio_parents[] = {"pll_audio"};
static const char *lcd0_parents[] = {"pll_video0","","","", "","","",""};
static const char *lcd1_parents[] = {"pll_video1","","","", "","","",""};
static const char *csi_s_parents[] = {"pll_periph", "", "", "", "", "pll_ve","",""};
static const char *csi_m_parents[] = {"pll_video0", "", "", "pll_de", "", "hosc","",""};
static const char *mbus_parents[] = {"hosc", "pll_periph", "pll_ddr",""};
static const char *ve_parents[] = {"pll_ve"};
static const char *gpu_parents[] = {"pll_gpu"};
static const char *gpumem_parents[] = {"pll_gpu","pll_periph"};
static const char *ahb1mod_parents[] = {"ahb1"};
static const char *apb1mod_parents[] = {"apb1"};
static const char *apb2mod_parents[] = {"apb2"};
static const char *sdmmc2_parents[] = {"sdmmc2mod"};
static const char *hdmi_parents[]= {"pll_video1","","",""};
static const char *mipidsi0_parents[]= {"pll_video0","","","",  "","","","",  "pll_video0","","","",  "","","",""}; //mipi_dsi10
static const char *mipidsi1_parents[]= {"hosc","","","",  "","","","",  "","pll_video0","","",  "","","",""}; //mipi_dsi1
static const char *cpurdev_parents[]  = {"losc", "hosc","",""};
static const char *lvds_parents[] = {"lcd0"};
static const char *de_parents[] = {"pll_de"};
static const char *gmac_parents[] = {"ahb1","ahb2","",""};
static const char* hsic_parents[] =  {"pll_hsic"};

struct sunxi_clk_comgate com_gates[]={
{"csi",      0,  0x3,    BUS_GATE_SHARE|RST_GATE_SHARE|MBUS_GATE_SHARE, 0},
{"mipi_dsi", 0,  0x3,    BUS_GATE_SHARE|RST_GATE_SHARE,                 0},
{"gpu",      0,  0x7,    BUS_GATE_SHARE|RST_GATE_SHARE,                 0},
};
/*
SUNXI_CLK_PERIPH(name,    mux_reg,    mux_shift, mux_width, div_reg,    div_mshift, div_mwidth, div_nshift, div_nwidth, gate_flags, enable_reg, reset_reg, bus_gate_reg, drm_gate_reg, enable_shift, reset_shift, bus_gate_shift, dram_gate_shift, lock,com_gate,com_gate_off)
*/
SUNXI_CLK_PERIPH(cluster0, CPU_CFG,   12,        1,         0,          0,          0,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cluster1, CPU_CFG,   28,        1,         0,          0,          0,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(axi0,     0,          0,        0,         CPU_CFG,    0,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(axi1,     0,          0,        0,         CPU_CFG,   16,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(pll_periphahb1,0,     0,        0,         AHB1_CFG,   6,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ahb1,    AHB1_CFG,   12,        2,         AHB1_CFG,   0,          0,          4,          2,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(apb1,    0,           0,        0,         AHB1_CFG,   8,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(apb2,    APB2_CFG,   24,        2,         APB2_CFG,   0,          5,         16,          2,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cci400, CCI400_CFG,  24,        2,         CCI400_CFG, 0,          2,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(nand,    NAND_CFG,   24,        2,         NAND_CFG,   0,          4,         16,          2,          0,          NAND_CFG,   AHB1_RST0, AHB1_GATE0,   0,           31,           13,          13,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(sdmmc0,  SD0_CFG,    24,        2,         SD0_CFG,    0,          4,         16,          2,          0,          SD0_CFG,    AHB1_RST0, AHB1_GATE0,   0,           31,            8,           8,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(sdmmc1,  SD1_CFG,    24,        2,         SD1_CFG,    0,          4,         16,          2,          0,          SD1_CFG,    AHB1_RST0, AHB1_GATE0,   0,           31,            9,           9,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(sdmmc2mod,SD2_CFG,   24,        2,         SD2_CFG,   30,          1,          0,          0,          0,          0,          0,         0,            0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(sdmmc2,  0,           0,        0,         SD2_CFG,    0,          4,         16,          2,          0,          SD2_CFG,    AHB1_RST0, AHB1_GATE0,   0,           31,           10,          10,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(ss,      SS_CFG,     24,        2,         SS_CFG,     0,          4,         16,          2,          0,          SS_CFG,     AHB1_RST0, AHB1_GATE0,   0,           31,            5,           5,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spi0,    SPI0_CFG,   24,        2,         SPI0_CFG,   0,          4,         16,          2,          0,          SPI0_CFG,   AHB1_RST0, AHB1_GATE0,   0,           31,           20,          20,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spi1,    SPI1_CFG,   24,        2,         SPI1_CFG,   0,          4,         16,          2,          0,          SPI1_CFG,   AHB1_RST0, AHB1_GATE0,   0,           31,           21,          21,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(i2s0,    0,           0,        0,         0,          0,          4,          0,          0,          0,          I2S0_CFG,   APB1_RST,  APB1_GATE,    0,           31,           12,          12,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(i2s1,    0,           0,        0,         0,          0,          4,          0,          0,          0,          I2S1_CFG,   APB1_RST,  APB1_GATE,    0,           31,           13,          13,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(i2s2,    0,           0,        0,         0,          0,          4,          0,          0,          0,          I2S2_CFG,   APB1_RST,  APB1_GATE,    0,           31,           14,          14,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(tdm,     0,           0,        0,         TDM_CFG,    0,          4,          0,          0,          0,          TDM_CFG,    APB1_RST,  APB1_GATE,    0,           31,           15,          15,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spdif,   0,           0,        0,         SPDIF_CFG,  0,          4,          0,          0,          0,          SPDIF_CFG,  APB1_RST,  APB1_GATE,    0,           31,            1,           1,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbohci, 0,           0,        0,         0,          0,          0,          0,          0,          0,          USB_CFG,    AHB1_RST0, AHB1_GATE0,   0,           16,           29,          29,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbhsic, 0,           0,        0,         0,          0,          0,          0,          0,          0,          USB_CFG,    USB_CFG,   USB_CFG,      0,           10,            2,          11,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbehci0, 0,          0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST0, AHB1_GATE0,   0,            0,           26,          26,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbehci1, 0,          0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST0, AHB1_GATE0,   0,            0,           27,          27,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbotg,  0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST0, AHB1_GATE0,   0,            0,           24,          24,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbphy0, 0,           0,        0,         0,          0,          0,          0,          0,          0,          USB_CFG,    USB_CFG,   0,            0,            8,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(usbphy1, 0,           0,        0,         0,          0,          0,          0,          0,          0,          USB_CFG,    USB_CFG,   0,            0,            9,            1,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(de,      0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST1, AHB1_GATE1,   0,            0,           12,          12,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(lcd0,    LCD0_CFG,   24,        2,         0,          0,          0,          0,          0,          0,          LCD0_CFG,   AHB1_RST1, AHB1_GATE1,   0,           31,            4,           4,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(lcd1,    LCD1_CFG,   24,        2,         LCD1_CFG,   0,          4,          0,          0,          0,          LCD1_CFG,   AHB1_RST1, AHB1_GATE1,   0,           31,            5,           5,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(mipi_csi, 0,          0,        0,         0,          0,          0,          0,          0,          0,          MIPICSI_CFG,0,         0,            0,           31,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(csi_misc, 0,  		   0,        0,         0,          0,          0,          0,          0,          0,          MIPICSI_CFG,0,         0,            0,           16,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(csi_s,   CSI_CFG,    24,        3,         CSI_CFG,   16,          4,          0,          0,          0,          CSI_CFG,    AHB1_RST1, AHB1_GATE1,   DRAM_GATE,   31,            8,           8,              1,               &clk_lock,&com_gates[0],    0);
SUNXI_CLK_PERIPH(csi_m,   CSI_CFG,     8,        3,         CSI_CFG,    0,          5,          0,          0,          0,          CSI_CFG,    AHB1_RST1, AHB1_GATE1,   DRAM_GATE,   15,            8,           8,              1,               &clk_lock,&com_gates[0],    1);
SUNXI_CLK_PERIPH(ve,      0,           0,        0,         VE_CFG,    16,          3,          0,          0,          0,          VE_CFG,     AHB1_RST1, AHB1_GATE1,   DRAM_GATE,   31,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(lvds,    0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST2,  0,           0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(avs,     0,           0,        0,         0,          0,          0,          0,          0,          0,          AVS_CFG,    0,          0,           0,           31,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(hdmi,	   HDMI_CFG,  24,        2,         HDMI_CFG,   0,          4,          0,          0,          0,          HDMI_CFG,   AHB1_RST1,  AHB1_GATE1,  AHB1_RST1,   31,           11,          11,             10,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(hdmi_slow,0,  		   0,        0,         0,          0,          0,          0,          0,          0,          HDMI_SLOW,  0,          0,          0,            31,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(mbus,    MBUS_CFG,   24,        2,         MBUS_CFG,   0,          3,          0,          0,          0,          MBUS_CFG,   0,          0,           0,           31,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(mipi_dsi0,MIPI_DSI0, 24,        4,         MIPI_DSI0,  0,          4,          0,          0,          0,          MIPI_DSI0,  AHB1_RST0,  AHB1_GATE0,  0,           31,            1,           1,              0,               &clk_lock,&com_gates[1],    0);
SUNXI_CLK_PERIPH(mipi_dsi1,MIPI_DSI1, 24,        4,         MIPI_DSI1,  0,          4,          0,          0,          0,          MIPI_DSI1,  AHB1_RST0,  AHB1_GATE0,  0,           31,            1,           1,              0,               &clk_lock,&com_gates[1],    1);
SUNXI_CLK_PERIPH(gpucore,  0,          0,        0,         GPU_CORE,   0,          3,          0,          0,          0,          GPU_CORE,   AHB1_RST1,  AHB1_GATE1,  0,           31,           20,          20,              0,               &clk_lock,&com_gates[2],    0);
SUNXI_CLK_PERIPH(gpumem,   GPU_MEM,   24,        1,         GPU_MEM,    0,          3,          0,          0,          0,          GPU_MEM,    AHB1_RST1,  AHB1_GATE1,  0,           31,           20,          20,              0,               &clk_lock,&com_gates[2],    1);
SUNXI_CLK_PERIPH(gpuhyd,   0,          0,        0,         GPU_HYD,    0,          3,          0,          0,          0,          GPU_HYD,    AHB1_RST1,  AHB1_GATE1,  0,           31,           20,          20,              0,               &clk_lock,&com_gates[2],    2);
SUNXI_CLK_PERIPH(gmac,    AHB2_CFG,    0,        2,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST0,  AHB1_GATE0,  0,            0,           17,          17,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(spinlock,0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST1,  AHB1_GATE1,  0,            0,           22,          22,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(msgbox,  0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST1,  AHB1_GATE1,  0,            0,           21,          21,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(dma,     0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          AHB1_RST0,  AHB1_GATE0,  0,            0,            6,           6,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(pio,     0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          0,          APB1_GATE,   0,            0,            0,           5,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi0,    0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,            0,           0,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi1,    0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,            1,           1,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi2,    0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,            2,           2,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart0,   0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,           16,          16,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart1,   0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,           17,          17,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart2,   0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,           18,          18,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart3,   0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,           19,          19,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(uart4,   0,           0,        0,         0,          0,          0,          0,          0,          0,          0,          APB2_RST,   APB2_GATE,   0,            0,           20,          20,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(cpurcir,  CPUS_CIR,  24,        2,         CPUS_CIR,   0,          4,          16,         2,          0,          CPUS_CIR,   CPUS_APB0_GATE,CPUS_APB0_RST,0,       31,            1,           1,              0,               &clk_lock,NULL,             0);
SUNXI_CLK_PERIPH(twi3,    0,           0,        0,         0,          0,          0,          0,          0,          0,          0,     CPUS_APB0_RST, CPUS_APB0_GATE,0,            0,            6,           6,              0,               &clk_lock,NULL,             0);

struct periph_init_data sunxi_periphs_init[] = {
    {"cluster0",     CLK_GET_RATE_NOCACHE,  cluster0_parents, ARRAY_SIZE(cluster0_parents),     &sunxi_clk_periph_cluster0},
    {"cluster1",     CLK_GET_RATE_NOCACHE,  cluster1_parents, ARRAY_SIZE(cluster1_parents),     &sunxi_clk_periph_cluster1},
    {"axi0",     0,       				axi0_parents,     ARRAY_SIZE(axi0_parents),     &sunxi_clk_periph_axi0},
    {"axi1",     0,       				axi1_parents,     ARRAY_SIZE(axi1_parents),     &sunxi_clk_periph_axi1},
    {"pll_periphahb1",CLK_IGNORE_SYNCBOOT,pll_periphahb1_parents,  ARRAY_SIZE(pll_periphahb1_parents),  &sunxi_clk_periph_pll_periphahb1},
    {"ahb1",     0,       				ahb1_parents,     ARRAY_SIZE(ahb1_parents),     &sunxi_clk_periph_ahb1},
    {"apb1",     0,       				apb1_parents,     ARRAY_SIZE(apb1_parents),     &sunxi_clk_periph_apb1},
    {"apb2",     0,       				apb2_parents,     ARRAY_SIZE(apb2_parents),     &sunxi_clk_periph_apb2},
    {"cci400",   0,       				cci400_parents,   ARRAY_SIZE(cci400_parents),   &sunxi_clk_periph_cci400},
    {"nand",     0,       				perph1_parents,   ARRAY_SIZE(perph1_parents),   &sunxi_clk_periph_nand},
    {"sdmmc0",   0,       				perph1_parents,   ARRAY_SIZE(perph1_parents),   &sunxi_clk_periph_sdmmc0},
    {"sdmmc1",   0,       				perph1_parents,   ARRAY_SIZE(perph1_parents),   &sunxi_clk_periph_sdmmc1},
    {"sdmmc2mod",0,         			perph1_parents,   ARRAY_SIZE(perph1_parents),   &sunxi_clk_periph_sdmmc2mod},
    {"sdmmc2",   0,       			    sdmmc2_parents,   ARRAY_SIZE(sdmmc2_parents),   &sunxi_clk_periph_sdmmc2},
    {"ss",       0,       				perph1_parents,   ARRAY_SIZE(perph1_parents),   &sunxi_clk_periph_ss},
    {"spi0",     0,       				perph1_parents,   ARRAY_SIZE(perph1_parents),   &sunxi_clk_periph_spi0},
    {"spi1",     0,       				perph1_parents,   ARRAY_SIZE(perph1_parents),   &sunxi_clk_periph_spi1},
    {"i2s0",     0,       				audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_i2s0},
    {"i2s1",     0,       				audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_i2s1},
    {"i2s2",     0,       				audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_i2s2},
    {"tdm",      0,       				audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_tdm},
    {"spdif",    0,       				audio_parents,    ARRAY_SIZE(audio_parents),    &sunxi_clk_periph_spdif},
    {"usbohci",  0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_usbohci},
    {"usbhsic",  0,       				hsic_parents,     ARRAY_SIZE(hsic_parents),     &sunxi_clk_periph_usbhsic},
    {"usbehci0", 0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_usbehci0},
    {"usbehci1", 0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_usbehci1},
    {"usbotg",   0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_usbotg},
    {"usbphy0",  0,       				hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_usbphy0},
    {"usbphy1",  0,       				hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_usbphy1},
    {"de",       0,       				de_parents,       ARRAY_SIZE(de_parents),       &sunxi_clk_periph_de},
    {"lcd0",     0,          		    lcd0_parents,     ARRAY_SIZE(lcd0_parents),     &sunxi_clk_periph_lcd0},
    {"lcd1",     0,       			    lcd1_parents,     ARRAY_SIZE(lcd1_parents),     &sunxi_clk_periph_lcd1},
    {"mipi_csi", 0,       			    hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_mipi_csi},
    {"csi_misc", 0,       			    hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_csi_misc},
    {"csi_s",    0,       				csi_s_parents,    ARRAY_SIZE(csi_s_parents),    &sunxi_clk_periph_csi_s},
    {"csi_m",    0,       				csi_m_parents,    ARRAY_SIZE(csi_m_parents),    &sunxi_clk_periph_csi_m},
    {"lvds",     0,       	  		    lvds_parents,     ARRAY_SIZE(lvds_parents),     &sunxi_clk_periph_lvds},
    {"ve",       0,       				ve_parents,       ARRAY_SIZE(ve_parents),       &sunxi_clk_periph_ve},
    {"avs",      0,       				hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_avs},
    {"hdmi",	 0,       			    hdmi_parents,     ARRAY_SIZE(hdmi_parents),     &sunxi_clk_periph_hdmi},
    {"hdmi_slow",0,       			    hosc_parents,     ARRAY_SIZE(hosc_parents),     &sunxi_clk_periph_hdmi_slow},
    {"mbus",     0,       				mbus_parents,     ARRAY_SIZE(mbus_parents),     &sunxi_clk_periph_mbus},
    {"mipi_dsi0",0,         			mipidsi0_parents, ARRAY_SIZE(mipidsi0_parents), &sunxi_clk_periph_mipi_dsi0},
    {"mipi_dsi1",0,       			    mipidsi1_parents, ARRAY_SIZE(mipidsi1_parents), &sunxi_clk_periph_mipi_dsi1},
    {"gpucore",	 CLK_IGNORE_AUTORESET,  gpu_parents,      ARRAY_SIZE(gpu_parents),      &sunxi_clk_periph_gpucore},
    {"gpumem",	 CLK_IGNORE_AUTORESET,  gpumem_parents,   ARRAY_SIZE(gpumem_parents),   &sunxi_clk_periph_gpumem},
    {"gpuhyd",	 CLK_IGNORE_AUTORESET,  gpu_parents,      ARRAY_SIZE(gpu_parents),      &sunxi_clk_periph_gpuhyd},
    {"gmac",     0,       				gmac_parents,     ARRAY_SIZE(gmac_parents),     &sunxi_clk_periph_gmac},
    {"spinlock", 0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_spinlock},
    {"msgbox",   0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_msgbox},
    {"dma",      0,       				ahb1mod_parents,  ARRAY_SIZE(ahb1mod_parents),  &sunxi_clk_periph_dma},
    {"pio",      0,       				apb1mod_parents,  ARRAY_SIZE(apb1mod_parents),  &sunxi_clk_periph_pio},
    {"twi0",     0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_twi0},
    {"twi1",     0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_twi1},
    {"twi2",     0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_twi2},
    {"uart0",    0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_uart0},
    {"uart1",    0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_uart1},
    {"uart2",    0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_uart2},
    {"uart3",    0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_uart3},
    {"uart4",    0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_uart4},
};

struct periph_init_data sunxi_periphs_cpus_init[] = {
    {"cpurcir",CLK_GET_RATE_NOCACHE,    cpurdev_parents,ARRAY_SIZE(cpurdev_parents),    &sunxi_clk_periph_cpurcir},
    {"twi3",     0,       				apb2mod_parents,  ARRAY_SIZE(apb2mod_parents),  &sunxi_clk_periph_twi3},
};
#ifdef CONFIG_COMMON_CLK_ENABLE_SYNCBOOT_EARLY
extern int clk_syncboot(void);
#endif
struct sunxi_reg_ops clk_regops={sunxi_smc_readl, sunxi_smc_writel};
void __init sunxi_init_clocks(void)
{
    int     i;
    struct clk *clk;
    struct factor_init_data *factor;
    struct periph_init_data *periph;

#ifdef CONFIG_SUNXI_CLK_DUMMY_DEBUG
		sunxi_clk_base = &dummy_reg[0]; 
		dummy_reg_init();
#else
    /* get clk register base address */
    sunxi_clk_base = IO_ADDRESS(0x01c20000);
    sunxi_clk_cpus_base = IO_ADDRESS(0x01f01400);
#endif
    sunxi_clk_factor_initlimits();
    /* register oscs */
    clk = clk_register_fixed_rate(NULL, "losc", NULL, CLK_IS_ROOT, 31250);
    clk_register_clkdev(clk, "losc", NULL);

    clk = clk_register_fixed_rate(NULL, "hosc", NULL, CLK_IS_ROOT, 24000000);
    clk_register_clkdev(clk, "hosc", NULL);

    sunxi_clk_get_factors_ops(&mipi_ops);
    mipi_ops.get_parent = get_parent_mipi;
    mipi_ops.set_parent = set_parent_mipi;
    /* register normal factors, based on sunxi factor framework */
    for(i=0; i<ARRAY_SIZE(sunxi_factos); i++) {
        factor = &sunxi_factos[i];
        factor->priv_regops = &clk_regops;        
        clk = sunxi_clk_register_factors(NULL,  sunxi_clk_base, &clk_lock,factor);
        clk_register_clkdev(clk, factor->name, NULL);
    }

    /* register fixed factors, based on clk-fixed-factor framework, such as pllx2 for ex. */
    clk = clk_register_fixed_factor(NULL, "pll_audiox8", "pll_audio", 0, 8, 1);
    clk_register_clkdev(clk, "pll_audiox8", NULL);

    clk = clk_register_fixed_factor(NULL, "pll_audiox4", "pll_audio", 0, 8, 2);
    clk_register_clkdev(clk, "pll_audiox4", NULL);

    clk = clk_register_fixed_factor(NULL, "pll_audiox2", "pll_audio", 0, 8, 4);
    clk_register_clkdev(clk, "pll_audiox2", NULL);

    clk = clk_register_fixed_factor(NULL, "pll_periphx2", "pll_periph", 0, 2, 1);
    clk_register_clkdev(clk, "pll_periphx2", NULL);

    clk = clk_register_fixed_factor(NULL, "ahb2", "pll_periph", 0, 1, 2);
    clk_register_clkdev(clk, "ahb2", NULL);

    clk = clk_register_fixed_factor(NULL, "hoscd2", "hosc", 0, 1, 2);
    clk_register_clkdev(clk, "hoscd2", NULL);

    /* register periph clock */
    for(i=0; i<ARRAY_SIZE(sunxi_periphs_init); i++) {
        periph = &sunxi_periphs_init[i];
        periph->periph->priv_regops = &clk_regops;       
		clk = sunxi_clk_register_periph(periph->name, periph->parent_names,
					periph->num_parents,periph->flags, sunxi_clk_base, periph->periph);
        clk_register_clkdev(clk, periph->name, NULL);
    }
    for(i=0; i<ARRAY_SIZE(sunxi_periphs_cpus_init); i++) {
        periph = &sunxi_periphs_cpus_init[i];
        periph->periph->priv_regops = &clk_regops;      
        clk = sunxi_clk_register_periph(periph->name, periph->parent_names,
                        periph->num_parents,periph->flags, sunxi_clk_cpus_base , periph->periph);
        clk_register_clkdev(clk, periph->name, NULL);
        }
    clk_add_alias("pll1",NULL,"pll_cpu0",NULL);
    clk_add_alias("pll2",NULL,"pll_audio",NULL);
    clk_add_alias("pll3",NULL,"pll_video0",NULL);
    clk_add_alias("pll4",NULL,"pll_ve",NULL);
    clk_add_alias("pll5",NULL,"pll_ddr",NULL);
    clk_add_alias("pll6",NULL,"pll_periph",NULL);
    clk_add_alias("pll8",NULL,"pll_gpu",NULL);
    clk_add_alias("pll9",NULL,"pll_hsic",NULL);
    clk_add_alias("pll10",NULL,"pll_de",NULL);
    clk_add_alias("pll6ahb1",NULL,"pll_periphahb1",NULL);
#ifdef CONFIG_COMMON_CLK_ENABLE_SYNCBOOT_EARLY
	clk_syncboot();
#endif
}

