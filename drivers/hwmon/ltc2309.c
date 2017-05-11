/*
 * ltc2309.c - Driver for Linear Technology LTC2309 8-channel A/D converter
 * (C) 2017 Sam Povilus
 *
 * This driver is based on the lm75 and other lm_sensors/hwmon drivers
 *
 * Written by Sam Povilus <kernel.development@povil.us>
 *
 * For further information, see the Documentation/hwmon/ltc2309 file.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/printk.h>

/* The LTC2309 registers */
#define LTC2309_CMD_REG 0
#define LTC2309_DATA_REG 0

#define LTC2309_INT_VREF_MV	2500	/* Internal vref is 2.5V, 2500mV */
#define LTC2309_EXT_VREF_MV_MIN	-300	/* External vref min value gnd - 0.3 */
#define LTC2309_EXT_VREF_MV_MAX	6300	/* External vref max value Vdd + 0.3 */

#define LTC2309_CMD_SD_SE	0x80	/* Single ended inputs */
#define LTC2309_CMD_UNIPOLAR	0x08	/* Single ended inputs */

/* List of supported devices */
enum ltc2309_chips { ltc2309 };

/* Client specific data */
struct ltc2309_data {
	struct i2c_client *client;
/*	struct regmap *regmap;*/
	u8 cmd_byte;			/* Command byte without channel bits */
};

/* Command byte C2,C1,C0 - see datasheet */

static inline u8 ltc2309_cmd_byte(u8 cmd, int ch)
{
	return cmd | (((ch >> 1) | (ch & 0x01) << 2) << 4);
}

/* sysfs callback function */
static ssize_t ltc2309_show_in(struct device *dev, struct device_attribute *da,
			       char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct ltc2309_data *data = dev_get_drvdata(dev);
	u8 cmd = ltc2309_cmd_byte(data->cmd_byte, attr->index);
	s32 ret;

/*	ret = i2c_smbus_write_byte(data->client, cmd);
	if(ret < 0)
	{
		pr_err("could not write to device\n");
		}*/
	pr_info("wote 0001 0x%x\n",cmd);
	/* As far as I can tell the command word will be ignored */
	ret = i2c_smbus_read_word_data(data->client, cmd);
	if (ret < 0)
	{
		pr_err("could not read from device");
	}
	pr_info("read 0x%x\n",ret);
	/*ret = ret >> 4;*/
	/*return sprintf(buf, "%d\n",
	  DIV_ROUND_CLOSEST(ret * 4096, 1000));*/
	return sprintf(buf, "0x%x\n",ret);
}

static SENSOR_DEVICE_ATTR(in0_input, S_IRUGO, ltc2309_show_in, NULL, 0);
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, ltc2309_show_in, NULL, 1);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, ltc2309_show_in, NULL, 2);
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, ltc2309_show_in, NULL, 3);
static SENSOR_DEVICE_ATTR(in4_input, S_IRUGO, ltc2309_show_in, NULL, 4);
static SENSOR_DEVICE_ATTR(in5_input, S_IRUGO, ltc2309_show_in, NULL, 5);
static SENSOR_DEVICE_ATTR(in6_input, S_IRUGO, ltc2309_show_in, NULL, 6);
static SENSOR_DEVICE_ATTR(in7_input, S_IRUGO, ltc2309_show_in, NULL, 7);

static struct attribute *ltc2309_attrs[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in7_input.dev_attr.attr,
	NULL
};

ATTRIBUTE_GROUPS(ltc2309);

static int ltc2309_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct ltc2309_data *data;
	struct device *hwmon_dev;
	unsigned int vref_mv = LTC2309_INT_VREF_MV;

	data = devm_kzalloc(dev, sizeof(struct ltc2309_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;

	/* Bound Vref with min/max values */
	vref_mv = clamp_val(vref_mv, LTC2309_EXT_VREF_MV_MIN,
			    LTC2309_EXT_VREF_MV_MAX);


	hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
							   data,
							   ltc2309_groups);
	if(1)
	{
		data->cmd_byte |= LTC2309_CMD_SD_SE;
		data->cmd_byte |= LTC2309_CMD_UNIPOLAR;
	}
	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id ltc2309_device_ids[] = {
	{ "ltc2309", ltc2309 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ltc2309_device_ids);

static struct i2c_driver ltc2309_driver = {
	.driver = {
		.name = "ltc2309",
	},

	.id_table = ltc2309_device_ids,
	.probe = ltc2309_probe,
};

module_i2c_driver(ltc2309_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sam Povilus <kernel.development@povil.us>");
MODULE_DESCRIPTION("Driver for Linear Technology LTC2309 8-channel A/D converter");
