/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/cpr-regulator.h>

#include <mach/clk-provider.h>
#include <mach/msm_bus.h>
#include <mach/msm_bus_board.h>
#include <mach/rpm-regulator-smd.h>
#include <mach/socinfo.h>

#include "acpuclock-cortex.h"

#define RCG_CONFIG_UPDATE_BIT		BIT(0)

static struct acpuclk_drv_data *priv;
void __iomem *apcs_rcg_config;

static struct msm_bus_paths bw_level_tbl_8226[] = {
	[0] =  BW_MBPS(152), /* At least 19 MHz on bus. */
	[1] =  BW_MBPS(300), /* At least 37.5 MHz on bus. */
	[2] =  BW_MBPS(400), /* At least 50 MHz on bus. */
	[3] =  BW_MBPS(800), /* At least 100 MHz on bus. */
	[4] = BW_MBPS(1600), /* At least 200 MHz on bus. */
	[5] = BW_MBPS(2664), /* At least 333 MHz on bus. */
	[6] = BW_MBPS(3200), /* At least 400 MHz on bus. */
	[7] = BW_MBPS(4264), /* At least 533 MHz on bus. */
};

static struct msm_bus_paths bw_level_tbl_8610[] = {
	[0] =  BW_MBPS(152), /* At least 19 MHz on bus. */
	[1] =  BW_MBPS(300), /* At least 37.5 MHz on bus. */
	[2] =  BW_MBPS(400), /* At least 50 MHz on bus. */
	[3] =  BW_MBPS(800), /* At least 100 MHz on bus. */
	[4] = BW_MBPS(1600), /* At least 200 MHz on bus. */
	[5] = BW_MBPS(2664), /* At least 333 MHz on bus. */
};

static struct msm_bus_scale_pdata bus_client_pdata = {
	.usecase = bw_level_tbl_8226,
	.num_usecases = ARRAY_SIZE(bw_level_tbl_8226),
	.active_only = 1,
	.name = "acpuclock",
};

/*
struct clkctl_acpu_speed {
        bool use_for_scaling;
        unsigned int khz;
        int src;
        unsigned int src_sel;
        unsigned int src_div;
        unsigned int vdd_cpu;
        unsigned int vdd_mem;
        unsigned int bw_level;
};
*/

static struct clkctl_acpu_speed acpu_freq_tbl_8226_1p1[] = {
#ifdef CONFIG_CPU_UNDERCLOCK
	{ 1,  192000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 3 },
	{ 1,  249600, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
#endif
	{ 1,  300000, PLL0,    4, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  384000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  600000, PLL0,    4, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  787200, ACPUPLL, 5, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  998400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1094400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1190400, ACPUPLL, 5, 0,   CPR_CORNER_7,   0, 7 },
	{ 1, 1305600, ACPUPLL, 5, 0,   CPR_CORNER_8,   0, 7 },
	{ 1, 1344000, ACPUPLL, 5, 0,   CPR_CORNER_9,   0, 7 },
	{ 1, 1401600, ACPUPLL, 5, 0,   CPR_CORNER_10,  0, 7 },
	{ 1, 1497600, ACPUPLL, 5, 0,   CPR_CORNER_11,  0, 7 },
	{ 1, 1593600, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 0 }
};

#ifdef CONFIG_USERSPACE_VOLTAGE_CONTROL
static struct clkctl_acpu_speed acpu_freq_tbl_8226_1p2[] = {
#ifdef CONFIG_CPU_UNDERCLOCK
	{ 1,  192000, ACPUPLL, 5, 2,   1140000,    1140000, 3 },
	{ 1,  249600, ACPUPLL, 5, 2,   1140000,    1140000, 4 },
#endif
	{ 1,  300000, PLL0,    4, 2,   1140000,    1140000, 4 },
	{ 1,  384000, ACPUPLL, 5, 2,   1140000,    1140000, 4 },
	{ 1,  600000, PLL0,    4, 0,   1140000,    1140000, 6 },
	{ 1,  787200, ACPUPLL, 5, 0,   1140000,    1140000, 6 },
	{ 1,  998400, ACPUPLL, 5, 0,   1150000,    1150000, 7 },
	{ 1, 1094400, ACPUPLL, 5, 0,   1150000,    1150000, 7 },
	{ 1, 1190400, ACPUPLL, 5, 0,   1150000,    1150000, 7 },
	{ 1, 1305600, ACPUPLL, 5, 0,   1280000,    1280000, 7 },
	{ 1, 1344000, ACPUPLL, 5, 0,   1280000,    1280000, 7 },
	{ 1, 1401600, ACPUPLL, 5, 0,   1280000,    1280000, 7 },
	{ 1, 1497600, ACPUPLL, 5, 0,   1280000,    1280000, 7 },
	{ 1, 1593600, ACPUPLL, 5, 0,   1280000,    1280000, 7 },
	{ 0 }
};
/* Carlos Jesús (KLOZZ OR TEAMMEX@XDA-Developers)	
 * TEAMMEX NOTE: for _USERSPACE_VOLTAGE_CONTROL_
 *
 * VOLTAGE Added SOME CONFIGS so this need more test I add lowe voltage 
 * Because Stock uses 1140mV I added FOR test 1280mV in Higher Freq
 * But if work fine I make DOWN to 1150mv for Baterry saver
 * Maybe this fix the Issue on Some Kernel Tweakers :)
 */
#else

static struct clkctl_acpu_speed acpu_freq_tbl_8226_1p2[] = {
#ifdef CONFIG_CPU_UNDERCLOCK
	{ 1,  192000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 3 },
	{ 1,  249600, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
#endif
	{ 1,  300000, PLL0,    4, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  384000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  600000, PLL0,    4, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  787200, ACPUPLL, 5, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  998400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1094400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1190400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 0 }
};

#endif

static struct clkctl_acpu_speed acpu_freq_tbl_8226_1p4[] = {
#ifdef CONFIG_CPU_UNDERCLOCK
	{ 1,  192000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 3 },
	{ 1,  249600, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
#endif
	{ 1,  300000, PLL0,    4, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  384000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  600000, PLL0,    4, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  787200, ACPUPLL, 5, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  998400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1094400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1190400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1305600, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1344000, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 1, 1401600, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 0 }
};

static struct clkctl_acpu_speed acpu_freq_tbl_8226_1p6[] = {
#ifdef CONFIG_CPU_UNDERCLOCK
	{ 1,  192000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 3 },
	{ 1,  249600, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
#endif
	{ 1,  300000, PLL0,    4, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  384000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 4 },
	{ 1,  600000, PLL0,    4, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  787200, ACPUPLL, 5, 0,   CPR_CORNER_4,   0, 6 },
	{ 1,  998400, ACPUPLL, 5, 0,   CPR_CORNER_5,   0, 7 },
	{ 1, 1094400, ACPUPLL, 5, 0,   CPR_CORNER_6,   0, 7 },
	{ 1, 1190400, ACPUPLL, 5, 0,   CPR_CORNER_7,   0, 7 },
	{ 1, 1305600, ACPUPLL, 5, 0,   CPR_CORNER_8,   0, 7 },
	{ 1, 1344000, ACPUPLL, 5, 0,   CPR_CORNER_9,   0, 7 },
	{ 1, 1401600, ACPUPLL, 5, 0,   CPR_CORNER_10,  0, 7 },
	{ 1, 1497600, ACPUPLL, 5, 0,   CPR_CORNER_11,  0, 7 },
	{ 1, 1593600, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 0 }
};

static struct clkctl_acpu_speed acpu_freq_tbl_8610[] = {
	{ 1,  300000, PLL0,    4, 2,   CPR_CORNER_2,   0, 3 },
	{ 1,  384000, ACPUPLL, 5, 2,   CPR_CORNER_2,   0, 3 },
	{ 1,  600000, PLL0,    4, 0,   CPR_CORNER_4,   0, 4 },
	{ 1,  787200, ACPUPLL, 5, 0,   CPR_CORNER_4,   0, 4 },
	{ 1,  998400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 5 },
	{ 1, 1190400, ACPUPLL, 5, 0,   CPR_CORNER_12,  0, 7 },
	{ 0 }
};

static struct clkctl_acpu_speed *pvs_tables_8226[NUM_SPEED_BIN] = {
	[0] = acpu_freq_tbl_8226_1p2,
	[6] = acpu_freq_tbl_8226_1p2,
	[2] = acpu_freq_tbl_8226_1p4,
	[5] = acpu_freq_tbl_8226_1p4,
	[4] = acpu_freq_tbl_8226_1p4,
	[7] = acpu_freq_tbl_8226_1p6,
	[1] = acpu_freq_tbl_8226_1p6,
};

static struct acpuclk_drv_data drv_data = {
	.freq_tbl = acpu_freq_tbl_8226_1p1,
	.pvs_tables = pvs_tables_8226,
	.bus_scale = &bus_client_pdata,
#ifdef CONFIG_USERSPACE_VOLTAGE_CONTROL
	.vdd_max_cpu = 1280000,
#else
	.vdd_max_cpu = CPR_CORNER_12,
#endif
	.src_clocks = {
		[PLL0].name = "gpll0",
		[ACPUPLL].name = "a7sspll",
	},
	.reg_data = {
		.cfg_src_mask = BM(10, 8),
		.cfg_src_shift = 8,
		.cfg_div_mask = BM(4, 0),
		.cfg_div_shift = 0,
		.update_mask = RCG_CONFIG_UPDATE_BIT,
		.poll_mask = RCG_CONFIG_UPDATE_BIT,
	},
#ifdef CONFIG_CPU_UNDERCLOCK
	.power_collapse_khz = 192000,
	.wait_for_irq_khz = 192000,
#else
	.power_collapse_khz = 300000,
	.wait_for_irq_khz = 300000,
#endif
};

#define ATTR_RW(_name)  \
        static struct kobj_attribute _name##_interface = __ATTR(_name, 0644, _name##_show, _name##_store);

#define ATTR_RO(_name)  \
        static struct kobj_attribute _name##_interface = __ATTR(_name, 0444, _name##_show, NULL);

#define ATTR_WO(_name)  \
        static struct kobj_attribute _name##_interface = __ATTR(_name, 0200, _name##_show, NULL);

static struct kobject *acpupll_kobject;

static ssize_t acpupll_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	//struct clkctl_acpu_speed *f;
	//struct acpuclk_reg_data *r;
	//void __iomem *apcs_rcg_config;
	/*struct clk *parent;*/
	u32 regval/*, div, src*/;
	unsigned long /*rate, */count;
	//char tmp_buf[100];
/*
	if (IS_ERR_OR_NULL(priv)) {
		pr_err("priv is bad");
		return -EINVAL;
	}

	f = priv->freq_tbl;
	r = &priv->reg_data;

	if (IS_ERR_OR_NULL(r)) {
		pr_err("regdata is bad");
		return -EINVAL;
	}
*/
	//apcs_rcg_config = priv->apcs_rcg_config;

	regval = readl_relaxed(apcs_rcg_config);
/*
	src = regval & r->cfg_src_mask;
	src >>= r->cfg_src_shift;

	div = regval & r->cfg_div_mask;
	div >>= r->cfg_div_shift;
	div = div > 1 ? (div + 1) / 2 : 0;
*/
/*
	for (f = priv->freq_tbl; f->khz; f++) {
		if (f->src_sel != src || f->src_div != div)
			continue;

		parent = priv->src_clocks[f->src].clk;
		rate = parent->rate / (div ? div : 1);
		if (f->khz * 1000 == rate)
			break;
	}
*/
	//count = sprintf(tmp_buf, "src: %#010x, div: %#010x\n", src, div);
	//count += sprintf(buf, "regval: %#010x (%d kHz)\n%s", regval, f->khz, tmp_buf);
	//count += sprintf(buf, "regval: %#010x\n%s", regval, tmp_buf);
	count = sprintf(buf, "regval: %#010x\n", regval);

	return count;
}
ATTR_RO(acpupll);

static struct attribute *acpupll_attrs[] = {
	&acpupll_interface.attr,
	NULL,
};

static struct attribute_group acpupll_interface_group = {
	.attrs = acpupll_attrs,
};

static int __init acpuclk_a7_probe(struct platform_device *pdev)
{
	struct resource *res;
	u32 i;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rcg_base");
	if (!res)
		return -EINVAL;

	drv_data.apcs_rcg_cmd = devm_ioremap(&pdev->dev, res->start,
		resource_size(res));
	if (!drv_data.apcs_rcg_cmd)
		return -ENOMEM;

	drv_data.apcs_rcg_config = drv_data.apcs_rcg_cmd + 4;
	apcs_rcg_config = drv_data.apcs_rcg_config;
	pr_err("apcs_rcg_config: %#010x\n", (u32)apcs_rcg_config);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pte_efuse");
	if (res) {
		drv_data.pte_efuse_base = devm_ioremap(&pdev->dev, res->start,
			resource_size(res));
		if (!drv_data.pte_efuse_base)
			return -ENOMEM;
	}

	drv_data.vdd_cpu = devm_regulator_get(&pdev->dev, "a7_cpu");
	if (IS_ERR(drv_data.vdd_cpu)) {
		dev_err(&pdev->dev, "regulator for %s get failed\n", "a7_cpu");
		return PTR_ERR(drv_data.vdd_cpu);
	}

	for (i = 0; i < NUM_SRC; i++) {
		if (!drv_data.src_clocks[i].name)
			continue;
		drv_data.src_clocks[i].clk =
			devm_clk_get(&pdev->dev, drv_data.src_clocks[i].name);
		if (IS_ERR(drv_data.src_clocks[i].clk)) {
			dev_err(&pdev->dev, "Unable to get clock %s\n",
				drv_data.src_clocks[i].name);
			return -EPROBE_DEFER;
		}
	}

	/* Enable the always on source */
	clk_prepare_enable(drv_data.src_clocks[PLL0].clk);
	priv = &drv_data;

	return acpuclk_cortex_init(pdev, &drv_data);
}

static struct of_device_id acpuclk_a7_match_table[] = {
	{.compatible = "qcom,acpuclk-a7"},
	{}
};

static struct platform_driver acpuclk_a7_driver = {
	.driver = {
		.name = "acpuclk-a7",
		.of_match_table = acpuclk_a7_match_table,
		.owner = THIS_MODULE,
	},
};

void msm8610_acpu_init(void)
{
	drv_data.bus_scale->usecase = bw_level_tbl_8610;
	drv_data.bus_scale->num_usecases = ARRAY_SIZE(bw_level_tbl_8610);
	drv_data.freq_tbl = acpu_freq_tbl_8610;
}

static int __init acpuclk_a7_init(void)
{
	int rc;

	if (cpu_is_msm8610())
		msm8610_acpu_init();

        acpupll_kobject = kobject_create_and_add("acpupll", kernel_kobj);
        if (!acpupll_kobject) {
                pr_err("[acpupll] failed to create kobject interface\n");
        }

        rc = sysfs_create_group(acpupll_kobject, &acpupll_interface_group);
        if (rc)
                kobject_put(acpupll_kobject);

	return platform_driver_probe(&acpuclk_a7_driver, acpuclk_a7_probe);
}
device_initcall(acpuclk_a7_init);
