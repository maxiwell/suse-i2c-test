// SPDX-License-Identifier: GPL-2.0-or-later
/*
    i2c-stub.c - Another I2C chip emulator

*/

#define DEBUG 1
#define pr_fmt(fmt) "i2c-stub: " fmt

#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c-dev.h>
#include <linux/bcd.h>

#define CHIP_ADDR 0x24
#define FIRMWARE_VERSION 0x14
#define N_REG 256

#define I2C_MSG_READ (msg->flags & I2C_M_RD)

enum stub_cmds {
	ID = 0,
	GET_FIRMWARE_VERSION = 1,
};

struct stub_chip {
	u16 registers[N_REG];
	u16 firmware_version;
};
struct stub_chip chip;

struct stub_request {
	int cmd;
	u32 buf_response[N_REG];
	u32 buf_len;
};

static int stub_handle_response(struct i2c_adapter *adap, struct i2c_msg *msg,
				struct stub_request *request)
{
	int i;
	int ret = 0;

	if (msg->len != request->buf_len)
		return -EINVAL;

	for (i = 0; i < msg->len; i++)
		msg->buf[i] = request->buf_response[i];

	return ret;
}

static int stub_handle_cmds(struct i2c_adapter *adap, struct i2c_msg *msg,
			struct stub_request *request)
{
	int ret = 0;
	switch (msg->buf[0]) {
		case ID:
			if (msg->len > 1) {
				ret = -EOPNOTSUPP;
				break;
			}

			// Reply: ss xx yy
			//  ss is the status byte
			//  xx is the major version
			//  yy is the minor version
			
			request->cmd = msg->buf[0];
			request->buf_len = 3;

			request->buf_response[0] = 0;
			request->buf_response[1] = bin2bcd(I2C_MAJOR);
			request->buf_response[2] = bin2bcd(adap->nr);

			break;

		case GET_FIRMWARE_VERSION:
			request->cmd = msg->buf[0];
			request->buf_len = 1;

			request->buf_response[0] = chip.firmware_version;
			break;
		default:
			dev_dbg(&adap->dev, "Unsupported command\n");
			ret = -EOPNOTSUPP;
			break;
	}
	return ret;
}

static int stub_handle_msg(struct i2c_adapter *adap, struct i2c_msg *msg,
			struct stub_request *request)
{
	int ret = 0;

	if (msg->addr != CHIP_ADDR)
		return -ENODEV;

	if (I2C_MSG_READ)
		ret = stub_handle_response(adap, msg, request);
	else
		ret = stub_handle_cmds(adap, msg, request);

	return ret;
}

static int stub_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct stub_request request;
	int i;
	int ret = 0;

	request.buf_len = 0;
	for (i = 0; i < num; i++) {
		ret = stub_handle_msg(adap, &msgs[i], &request);
		if (ret < 0)
			break;
	}
	
	/* Return the number of messages successfully
	 * processed, or a negative value on error */	
	return ret < 0 ? ret : num;
}

static u32 stub_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}

static const struct i2c_algorithm smbus_algorithm = {
	.functionality	= stub_func,
	.master_xfer	= stub_xfer,
};

static struct i2c_adapter stub_adapter = {
	.owner		= THIS_MODULE,
	.class		= I2C_CLASS_HWMON | I2C_CLASS_SPD,
	.algo		= &smbus_algorithm,
	.name		= "Another I2C stub driver",
};

static int __init i2c_stub_init(void)
{
	int ret; 

	pr_info("Virtual chip at 0x%02x\n", CHIP_ADDR);

	memset(chip.registers, 0, sizeof(chip.registers));
	chip.firmware_version = FIRMWARE_VERSION;
	ret = i2c_add_adapter(&stub_adapter);

	return ret;
}

static void __exit i2c_stub_exit(void)
{
	i2c_del_adapter(&stub_adapter);
}

MODULE_AUTHOR("Maxiwell S. Garcia <maxiwell@gmail.com>");
MODULE_DESCRIPTION("Another I2C stub driver");
MODULE_LICENSE("GPL");

module_init(i2c_stub_init);
module_exit(i2c_stub_exit);
