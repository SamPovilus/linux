/*
 * ltc2309.c - driver for Liner Technology LTC2309 8-channel A/D converter 
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

/* The LTC2309 registers */
#define LTC2309_CMD_REG 0
#define LTC2309_DATA_REG 0

/* List of supported devices */
enum ltc2309_chips { ltc2309, ads7830 };

/* Client specific data */
struct ltc2309_data {
	struct regmap *regmap;
	u8 cmd_byte;			/* Command byte without channel bits */
	unsigned int lsb_resol;		/* Resolution of the ADC sample LSB */
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
	unsigned int regval;
	int err;
	s32 i2c_smbus_write_byte(const struct i2c_client *client, u8 value)

	i2c_smbus_write_word_data();
	err = regmap_write(data->regmap, LTC2309_CMD_REG, cmd);
	
	err = regmap_read(data->regmap, LTC2309_DATA_REG, &regval);
	if (err < 0)
		return err;

	return sprintf(buf, "%d\n",
		       DIV_ROUND_CLOSEST(regval * data->lsb_resol, 1000));
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

static const struct regmap_config ads2828_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
};

static const struct regmap_config ads2830_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int ltc2309_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct ltc2309_platform_data *pdata = dev_get_platdata(dev);
	struct ltc2309_data *data;
	struct device *hwmon_dev;
	unsigned int vref_mv = LTC2309_INT_VREF_MV;
	bool diff_input = false;
	bool ext_vref = false;
	unsigned int regval;

	data = devm_kzalloc(dev, sizeof(struct ltc2309_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (pdata) {
		diff_input = pdata->diff_input;
		ext_vref = pdata->ext_vref;
		if (ext_vref && pdata->vref_mv)
			vref_mv = pdata->vref_mv;
	}

	/* Bound Vref with min/max values */
	vref_mv = clamp_val(vref_mv, LTC2309_EXT_VREF_MV_MIN,
			    LTC2309_EXT_VREF_MV_MAX);

	/* LTC2309 uses 12-bit samples, while ADS7830 is 8-bit */
	if (id->driver_data == ltc2309) {
		data->lsb_resol = DIV_ROUND_CLOSEST(vref_mv * 1000, 4096);
		data->regmap = devm_regmap_init_i2c(client,
						    &ads2828_regmap_config);
	} else {
		data->lsb_resol = DIV_ROUND_CLOSEST(vref_mv * 1000, 256);
		data->regmap = devm_regmap_init_i2c(client,
						    &ads2830_regmap_config);
	}

	if (IS_ERR(data->regmap))
		return PTR_ERR(data->regmap);

	data->cmd_byte = ext_vref ? LTC2309_CMD_PD1 : LTC2309_CMD_PD3;
	if (!diff_input)
		data->cmd_byte |= LTC2309_CMD_SD_SE;

	/*
	 * Datasheet specifies internal reference voltage is disabled by
	 * default. The internal reference voltage needs to be enabled and
	 * voltage needs to settle before getting valid ADC data. So perform a
	 * dummy read to enable the internal reference voltage.
	 */
	if (!ext_vref)
		regmap_read(data->regmap, data->cmd_byte, &regval);

	hwmon_dev = devm_hwmon_device_register_with_groups(dev, client->name,
							   data,
							   ltc2309_groups);
	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id ltc2309_device_ids[] = {
	{ "ltc2309", ltc2309 },
	{ "ads7830", ads7830 },
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
MODULE_AUTHOR("Steve Hardy <shardy@redhat.com>");
MODULE_DESCRIPTION("Driver for TI LTC2309 A/D converter and compatibles");
