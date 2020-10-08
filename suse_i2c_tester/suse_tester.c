// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * suse_tester.c - handle most I2C EEPROMs
 *
 * Copyright (C) 2005-2007 David Brownell
 * Copyright (C) 2008 Wolfram Sang, Pengutronix
 */

#include <linux/acpi.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nvmem-provider.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>

#define GET_FIRMWARE_VERSION 1

static int suse_tester_get_firm_ver(struct i2c_client *client, int *firm_ver)
{
	int ret;

	u32 buf[1] = { GET_FIRMWARE_VERSION };
	struct i2c_msg msg[2] = {
		{ .addr = client->addr, .len = 1, .buf = (u8*) &buf },
		{ .addr = client->addr, .len = 1, .buf = (u8*) &buf, .flags = I2C_M_RD }
	};

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		pr_err("suse-i2c-tester: Failed to get firmware version\n");
		return ret;
	}

	*firm_ver = buf[0];
	return 0;
}

static int suse_tester_probe(struct i2c_client *client)
{
	int ret;
	int firm_ver;

	ret = suse_tester_get_firm_ver(client, &firm_ver);
	if (ret < 0)
		return ret;

	pr_info("suse-i2c-tester: Firmware version 0x%x\n", firm_ver);
	return 0;
}

static const struct of_device_id suse_tester_of_match[] = {
	{ .compatible = "suse,i2c-tester", },
	{ },
};
MODULE_DEVICE_TABLE(of, suse_tester_of_match);

static const struct i2c_device_id suse_tester_id[] = {
	{ "suse-i2c-tester", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, suse_tester_id);

static struct i2c_driver suse_tester_driver = {
	.driver = {
		.name = "suse-i2c-tester",
		.of_match_table = suse_tester_of_match,
	},
	.probe_new = suse_tester_probe,
	.id_table = suse_tester_id,
};
module_i2c_driver(suse_tester_driver);

MODULE_DESCRIPTION("Driver for I2C Tester");
MODULE_AUTHOR("Maxiwell S. Garcia <maxiwell@gmail.com>");
MODULE_LICENSE("GPL");
