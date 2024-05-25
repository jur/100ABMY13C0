/*
 * lp5523.c - LP5523 LED Driver
 *
 * Copyright (C) 2010 Nokia Corporation
 * Copyright (C) 2012 Texas Instruments
 *
 * Contact: Samu Onkalo <samu.p.onkalo@nokia.com>
 *          Milo(Woogyom) Kim <milo.kim@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_data/leds-lp55xx.h>
#include <linux/slab.h>

#include "leds-lp55xx-common.h"

#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
#define LP5523_PROGRAM_PAGES		6
#endif
#define LP5523_PROGRAM_LENGTH		32
#define LP5523_MAX_LEDS			9

/* Registers */
#define LP5523_REG_ENABLE		0x00
#define LP5523_REG_OP_MODE		0x01
#define LP5523_REG_ENABLE_LEDS_MSB	0x04
#define LP5523_REG_ENABLE_LEDS_LSB	0x05
#define LP5523_REG_LED_PWM_BASE		0x16
#define LP5523_REG_LED_CURRENT_BASE	0x26
#define LP5523_REG_CONFIG		0x36
#define LP5523_REG_STATUS		0x3A
#define LP5523_REG_RESET		0x3D
#define LP5523_REG_LED_TEST_CTRL	0x41
#define LP5523_REG_LED_TEST_ADC		0x42
#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
#define LP5523_REG_ENG1_START_ADDR	0x4C
#define LP5523_REG_ENG2_START_ADDR	0x4D
#define LP5523_REG_ENG3_START_ADDR	0x4E
#endif
#define LP5523_REG_PROG_PAGE_SEL	0x4F
#define LP5523_REG_PROG_MEM		0x50

/* Bit description in registers */
#define LP5523_ENABLE			0x40
#define LP5523_AUTO_INC			0x40
#define LP5523_PWR_SAVE			0x20
#define LP5523_PWM_PWR_SAVE		0x04
#define LP5523_CP_AUTO			0x18
#define LP5523_AUTO_CLK			0x02

#define LP5523_EN_LEDTEST		0x80
#define LP5523_LEDTEST_DONE		0x80
#define LP5523_RESET			0xFF
#define LP5523_ADC_SHORTCIRC_LIM	80
#define LP5523_EXT_CLK_USED		0x08

/* Memory Page Selection */
#define LP5523_PAGE_ENG1		0
#define LP5523_PAGE_ENG2		1
#define LP5523_PAGE_ENG3		2

/* Program Memory Operations */
#define LP5523_MODE_ENG1_M		0x30	/* Operation Mode Register */
#define LP5523_MODE_ENG2_M		0x0C
#define LP5523_MODE_ENG3_M		0x03
#define LP5523_LOAD_ENG1		0x10
#define LP5523_LOAD_ENG2		0x04
#define LP5523_LOAD_ENG3		0x01

#define LP5523_ENG1_IS_LOADING(mode)	\
	((mode & LP5523_MODE_ENG1_M) == LP5523_LOAD_ENG1)
#define LP5523_ENG2_IS_LOADING(mode)	\
	((mode & LP5523_MODE_ENG2_M) == LP5523_LOAD_ENG2)
#define LP5523_ENG3_IS_LOADING(mode)	\
	((mode & LP5523_MODE_ENG3_M) == LP5523_LOAD_ENG3)

#define LP5523_EXEC_ENG1_M		0x30	/* Enable Register */
#define LP5523_EXEC_ENG2_M		0x0C
#define LP5523_EXEC_ENG3_M		0x03
#define LP5523_EXEC_M			0x3F
#define LP5523_RUN_ENG1			0x20
#define LP5523_RUN_ENG2			0x08
#define LP5523_RUN_ENG3			0x02

enum lp5523_chip_id {
	LP5523,
	LP55231,
};

static inline void lp5523_wait_opmode_done(void)
{
	usleep_range(1000, 2000);
}

static void lp5523_set_led_current(struct lp55xx_led *led, u8 led_current)
{
	led->led_current = led_current;
	lp55xx_write(led->chip, LP5523_REG_LED_CURRENT_BASE + led->chan_nr,
		led_current);
}

static int lp5523_post_init_device(struct lp55xx_chip *chip)
{
	int ret;

	ret = lp55xx_write(chip, LP5523_REG_ENABLE, LP5523_ENABLE);
	if (ret)
		return ret;

	/* Chip startup time is 500 us, 1 - 2 ms gives some margin */
	usleep_range(1000, 2000);

	ret = lp55xx_write(chip, LP5523_REG_CONFIG,
			    LP5523_AUTO_INC | LP5523_PWR_SAVE |
			    LP5523_CP_AUTO | LP5523_AUTO_CLK |
			    LP5523_PWM_PWR_SAVE);
	if (ret)
		return ret;

	/* turn on all leds */
	ret = lp55xx_write(chip, LP5523_REG_ENABLE_LEDS_MSB, 0x01);
	if (ret)
		return ret;

	return lp55xx_write(chip, LP5523_REG_ENABLE_LEDS_LSB, 0xff);
}

static void lp5523_load_engine(struct lp55xx_chip *chip)
{
#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	unsigned int eng_start_addr = chip->pdata->eng_start_addr;
	u8 mask = 0;
	u8 val = 0;

	// always enable eng1
	mask |= LP5523_MODE_ENG1_M;
	val |= LP5523_LOAD_ENG1;
	lp55xx_write(chip, LP5523_REG_ENG1_START_ADDR, (eng_start_addr & 0xFF));
	//printk(KERN_INFO "lp5523_load_engine eng1_addr = %X\n", (eng_start_addr & 0xFF));

	if ((eng_start_addr >> 8) & 0xFF) {
		mask |= LP5523_MODE_ENG2_M;
		val |= LP5523_LOAD_ENG2;
		lp55xx_write(chip, LP5523_REG_ENG2_START_ADDR, ((eng_start_addr >> 8) & 0xFF));
		//printk(KERN_INFO "lp5523_load_engine eng2_addr = %X\n", ((eng_start_addr >> 8) & 0xFF));
	}

	if ((eng_start_addr >> 16) & 0xFF) {
		mask |= LP5523_MODE_ENG3_M;
		val |= LP5523_LOAD_ENG3;
		lp55xx_write(chip, LP5523_REG_ENG3_START_ADDR, ((eng_start_addr >> 16) & 0xFF));
		//printk(KERN_INFO "lp5523_load_engine eng3_addr = %X\n", ((eng_start_addr >> 16) & 0xFF));
	}

	//printk(KERN_INFO "lp5523_load_engine mask = %X, val = %X\n", mask, val);
	lp55xx_update_bits(chip, LP5523_REG_OP_MODE, mask, val);

	lp5523_wait_opmode_done();
#else
	enum lp55xx_engine_index idx = chip->engine_idx;
	u8 mask[] = {
		[LP55XX_ENGINE_1] = LP5523_MODE_ENG1_M,
		[LP55XX_ENGINE_2] = LP5523_MODE_ENG2_M,
		[LP55XX_ENGINE_3] = LP5523_MODE_ENG3_M,
	};

	u8 val[] = {
		[LP55XX_ENGINE_1] = LP5523_LOAD_ENG1,
		[LP55XX_ENGINE_2] = LP5523_LOAD_ENG2,
		[LP55XX_ENGINE_3] = LP5523_LOAD_ENG3,
	};

	u8 page_sel[] = {
		[LP55XX_ENGINE_1] = LP5523_PAGE_ENG1,
		[LP55XX_ENGINE_2] = LP5523_PAGE_ENG2,
		[LP55XX_ENGINE_3] = LP5523_PAGE_ENG3,
	};

	lp55xx_update_bits(chip, LP5523_REG_OP_MODE, mask[idx], val[idx]);

	lp5523_wait_opmode_done();

	lp55xx_write(chip, LP5523_REG_PROG_PAGE_SEL, page_sel[idx]);
#endif
}

static void lp5523_stop_engine(struct lp55xx_chip *chip)
{
	lp55xx_write(chip, LP5523_REG_OP_MODE, 0);
	lp5523_wait_opmode_done();
}

static void lp5523_turn_off_channels(struct lp55xx_chip *chip)
{
	int i;

	for (i = 0; i < LP5523_MAX_LEDS; i++)
		lp55xx_write(chip, LP5523_REG_LED_PWM_BASE + i, 0);
}

static void lp5523_run_engine(struct lp55xx_chip *chip, bool start)
{
	int ret;
	u8 mode;
	u8 exec;

	/* stop engine */
	if (!start) {
		lp5523_stop_engine(chip);
		lp5523_turn_off_channels(chip);
		return;
	}

	/*
	 * To run the engine,
	 * operation mode and enable register should updated at the same time
	 */

	ret = lp55xx_read(chip, LP5523_REG_OP_MODE, &mode);
	if (ret)
		return;

	ret = lp55xx_read(chip, LP5523_REG_ENABLE, &exec);
	if (ret)
		return;

	/* change operation mode to RUN only when each engine is loading */
	if (LP5523_ENG1_IS_LOADING(mode)) {
		mode = (mode & ~LP5523_MODE_ENG1_M) | LP5523_RUN_ENG1;
		exec = (exec & ~LP5523_EXEC_ENG1_M) | LP5523_RUN_ENG1;
	}

	if (LP5523_ENG2_IS_LOADING(mode)) {
		mode = (mode & ~LP5523_MODE_ENG2_M) | LP5523_RUN_ENG2;
		exec = (exec & ~LP5523_EXEC_ENG2_M) | LP5523_RUN_ENG2;
	}

	if (LP5523_ENG3_IS_LOADING(mode)) {
		mode = (mode & ~LP5523_MODE_ENG3_M) | LP5523_RUN_ENG3;
		exec = (exec & ~LP5523_EXEC_ENG3_M) | LP5523_RUN_ENG3;
	}

	lp55xx_write(chip, LP5523_REG_OP_MODE, mode);
	lp5523_wait_opmode_done();

	lp55xx_update_bits(chip, LP5523_REG_ENABLE, LP5523_EXEC_M, exec);
}

static int lp5523_update_program_memory(struct lp55xx_chip *chip,
					const u8 *data, size_t size)
{
#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	u8 pattern[LP5523_PROGRAM_LENGTH * LP5523_PROGRAM_PAGES] = {0};
#else
	u8 pattern[LP5523_PROGRAM_LENGTH] = {0};
#endif
	unsigned cmd;
	char c[3];
	int update_size;
	int nrchars;
	int offset = 0;
	int ret;
	int i;

	/* clear program memory before updating */
	#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	int j;
	for (j = 0; j < LP5523_PROGRAM_PAGES; j++) {
		lp55xx_write(chip, LP5523_REG_PROG_PAGE_SEL, j);
		for (i = 0; i < LP5523_PROGRAM_LENGTH; i++)
			lp55xx_write(chip, LP5523_REG_PROG_MEM + i, 0);
	}
	#else
	for (i = 0; i < LP5523_PROGRAM_LENGTH; i++)
		lp55xx_write(chip, LP5523_REG_PROG_MEM + i, 0);
	#endif

	i = 0;
#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	while ((offset < size - 1) && (i < LP5523_PROGRAM_LENGTH*LP5523_PROGRAM_PAGES*2)) {
#else
	while ((offset < size - 1) && (i < LP5523_PROGRAM_LENGTH)) {
#endif
		/* separate sscanfs because length is working only for %s */
		ret = sscanf(data + offset, "%2s%n ", c, &nrchars);
		if (ret != 1)
			goto err;

		ret = sscanf(c, "%2x", &cmd);
		if (ret != 1)
			goto err;

		pattern[i] = (u8)cmd;
		offset += nrchars;
		i++;
	}

	/* Each instruction is 16bit long. Check that length is even */
	if (i % 2)
		goto err;

	update_size = i;
	#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	j = 0;
	//printk(KERN_INFO "lp5523_update_program_memory\n");
	//printk(KERN_INFO "LP5523_REG_PROG_PAGE_SEL %X %d\n", LP5523_REG_PROG_PAGE_SEL, j);
	lp55xx_write(chip, LP5523_REG_PROG_PAGE_SEL, j);
	for (i = 0; i < update_size; i++) {
		if ( (i > 0)  && !(i % LP5523_PROGRAM_LENGTH)) {
			j++;
			if ( j == LP5523_PROGRAM_PAGES ) break;
			//printk(KERN_INFO "LP5523_REG_PROG_PAGE_SEL %X %d\n", LP5523_REG_PROG_PAGE_SEL, j);
			lp55xx_write(chip, LP5523_REG_PROG_PAGE_SEL, j);
		}
		//printk(KERN_INFO "%X %X\n", LP5523_REG_PROG_MEM + (i - j*LP5523_PROGRAM_LENGTH), pattern[i]);
		lp55xx_write(chip, LP5523_REG_PROG_MEM + (i - j*LP5523_PROGRAM_LENGTH), pattern[i]);
	}
	#else
	for (i = 0; i < update_size; i++)
		lp55xx_write(chip, LP5523_REG_PROG_MEM + i, pattern[i]);
	#endif

	return 0;

err:
	dev_err(&chip->cl->dev, "wrong pattern format\n");
	return -EINVAL;
}

static void lp5523_firmware_loaded(struct lp55xx_chip *chip)
{
	const struct firmware *fw = chip->fw;

#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	if (fw->size > (LP5523_PROGRAM_LENGTH * LP5523_PROGRAM_PAGES *2)) {
#else
	if (fw->size > LP5523_PROGRAM_LENGTH) {
#endif
		dev_err(&chip->cl->dev, "firmware data size overflow: %zu\n",
			fw->size);
		return;
	}

	/*
	 * Program momery sequence
	 *  1) set engine mode to "LOAD"
	 *  2) write firmware data into program memory
	 */

	lp5523_load_engine(chip);
	lp5523_update_program_memory(chip, fw->data, fw->size);
}

static ssize_t lp5523_selftest(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct lp55xx_led *led = i2c_get_clientdata(to_i2c_client(dev));
	struct lp55xx_chip *chip = led->chip;
	struct lp55xx_platform_data *pdata = chip->pdata;
	int i, ret, pos = 0;
	u8 status, adc, vdd;

	mutex_lock(&chip->lock);

	ret = lp55xx_read(chip, LP5523_REG_STATUS, &status);
	if (ret < 0)
		goto fail;

	/* Check that ext clock is really in use if requested */
	if (pdata->clock_mode == LP55XX_CLOCK_EXT) {
		if  ((status & LP5523_EXT_CLK_USED) == 0)
			goto fail;
	}

	/* Measure VDD (i.e. VBAT) first (channel 16 corresponds to VDD) */
	lp55xx_write(chip, LP5523_REG_LED_TEST_CTRL, LP5523_EN_LEDTEST | 16);
	usleep_range(3000, 6000); /* ADC conversion time is typically 2.7 ms */
	ret = lp55xx_read(chip, LP5523_REG_STATUS, &status);
	if (ret < 0)
		goto fail;

	if (!(status & LP5523_LEDTEST_DONE))
		usleep_range(3000, 6000); /* Was not ready. Wait little bit */

	ret = lp55xx_read(chip, LP5523_REG_LED_TEST_ADC, &vdd);
	if (ret < 0)
		goto fail;

	vdd--;	/* There may be some fluctuation in measurement */

	for (i = 0; i < LP5523_MAX_LEDS; i++) {
		/* Skip non-existing channels */
		if (pdata->led_config[i].led_current == 0)
			continue;

		/* Set default current */
		lp55xx_write(chip, LP5523_REG_LED_CURRENT_BASE + i,
			pdata->led_config[i].led_current);

		lp55xx_write(chip, LP5523_REG_LED_PWM_BASE + i, 0xff);
		/* let current stabilize 2 - 4ms before measurements start */
		usleep_range(2000, 4000);
		lp55xx_write(chip, LP5523_REG_LED_TEST_CTRL,
			     LP5523_EN_LEDTEST | i);
		/* ADC conversion time is 2.7 ms typically */
		usleep_range(3000, 6000);
		ret = lp55xx_read(chip, LP5523_REG_STATUS, &status);
		if (ret < 0)
			goto fail;

		if (!(status & LP5523_LEDTEST_DONE))
			usleep_range(3000, 6000);/* Was not ready. Wait. */

		ret = lp55xx_read(chip, LP5523_REG_LED_TEST_ADC, &adc);
		if (ret < 0)
			goto fail;

		if (adc >= vdd || adc < LP5523_ADC_SHORTCIRC_LIM)
			pos += sprintf(buf + pos, "LED %d FAIL\n", i);

		lp55xx_write(chip, LP5523_REG_LED_PWM_BASE + i, 0x00);

		/* Restore current */
		lp55xx_write(chip, LP5523_REG_LED_CURRENT_BASE + i,
			led->led_current);
		led++;
	}
	if (pos == 0)
		pos = sprintf(buf, "OK\n");
	goto release_lock;
fail:
	pos = sprintf(buf, "FAIL\n");

release_lock:
	mutex_unlock(&chip->lock);

	return pos;
}

static void lp5523_led_brightness_work(struct work_struct *work)
{
	struct lp55xx_led *led = container_of(work, struct lp55xx_led,
					      brightness_work);
	struct lp55xx_chip *chip = led->chip;

	mutex_lock(&chip->lock);
	lp55xx_write(chip, LP5523_REG_LED_PWM_BASE + led->chan_nr,
		     led->brightness);
	mutex_unlock(&chip->lock);
}

#if defined(CONFIG_LEDS_LP5523_PREDEFINED_PATTERNS)
static inline bool _is_pc_overflow(struct lp55xx_predef_pattern *ptn)
{
#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	return (ptn->size_r >= (LP5523_PROGRAM_LENGTH * LP5523_PROGRAM_PAGES));
#else
	return (ptn->size_r >= LP5523_PROGRAM_LENGTH ||
		ptn->size_g >= LP5523_PROGRAM_LENGTH ||
		ptn->size_b >= LP5523_PROGRAM_LENGTH);
#endif
}

#define LP5523_PATTERN_OFF 0
static int lp5523_run_predef_led_pattern(struct lp55xx_chip *chip, int mode)
{
	struct lp55xx_predef_pattern *ptn;
	int i;

	lp5523_run_engine(chip, false);
	if (mode == LP5523_PATTERN_OFF) {
		return 0;
	}

	ptn = chip->pdata->patterns + (mode - 1);
	if (!ptn || _is_pc_overflow(ptn)) {
		dev_err(&chip->cl->dev, "invalid pattern data\n");
		return -EINVAL;
	}

	// alwayse use engine1
	chip->engine_idx = LP55XX_ENGINE_1;
	#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	chip->pdata->eng_start_addr = ptn->eng_start_addr;
	#endif
	lp5523_load_engine(chip);
	#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	int j = 0;
	//printk(KERN_INFO "lp5523_run_predef_led_pattern\n");
	//printk(KERN_INFO "LP5523_REG_PROG_PAGE_SEL %X %d\n", LP5523_REG_PROG_PAGE_SEL, j);
	lp55xx_write(chip, LP5523_REG_PROG_PAGE_SEL, j);
	for (i = 0; i < ptn->size_r; i++) {
		if ( (i > 0)  && !(i % LP5523_PROGRAM_LENGTH)) {
			j++;
			if ( j == LP5523_PROGRAM_PAGES ) break;
			//printk(KERN_INFO "LP5523_REG_PROG_PAGE_SEL %X %d\n", LP5523_REG_PROG_PAGE_SEL, j);
			lp55xx_write(chip, LP5523_REG_PROG_PAGE_SEL, j);
		}
		//printk(KERN_INFO "%X %X\n", LP5523_REG_PROG_MEM + (i - j*LP5523_PROGRAM_LENGTH), ptn->r[i]);
		lp55xx_write(chip, LP5523_REG_PROG_MEM + (i - j*LP5523_PROGRAM_LENGTH), ptn->r[i]);
	}
	#else
	for (i = 0; i<ptn->size_r; i++)
		lp55xx_write(chip, LP5523_REG_PROG_MEM + i, ptn->r[i]);
	#endif

	/* Run engines */
	lp5523_run_engine(chip, true);

	return 0;
}

static ssize_t lp5523_store_pattern(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	struct lp55xx_led *led = i2c_get_clientdata(to_i2c_client(dev));
	struct lp55xx_chip *chip = led->chip;
	struct lp55xx_predef_pattern *ptn = chip->pdata->patterns;
	int num_patterns = chip->pdata->num_patterns;
	unsigned long mode;
	int ret;

	ret = kstrtoul(buf, 0, &mode);
	if (ret)
		return ret;

	if (mode > num_patterns || !ptn)
		return -EINVAL;

	mutex_lock(&chip->lock);
	ret = lp5523_run_predef_led_pattern(chip, mode);
	mutex_unlock(&chip->lock);

	if (ret)
		return ret;

	return len;
}

static DEVICE_ATTR(led_pattern, S_IWUSR, NULL, lp5523_store_pattern);
#endif // LEDS_LP5523_PREDEFINED_PATTERNS

#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
static ssize_t lp5523_show_eng_start_addr(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct lp55xx_led *led = i2c_get_clientdata(to_i2c_client(dev));
	struct lp55xx_chip *chip = led->chip;

	return sprintf(buf, "0x%X\n", chip->pdata->eng_start_addr);
}

static ssize_t lp5523_store_eng_start_addr(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t len)
{
	struct lp55xx_led *led = i2c_get_clientdata(to_i2c_client(dev));
	struct lp55xx_chip *chip = led->chip;
	unsigned int addr;
	int ret;

	ret = kstrtoul(buf, 16, &addr);
	if (ret)
		return ret;

	chip->pdata->eng_start_addr = addr;

	return len;
}
static DEVICE_ATTR(eng_start_addr, S_IRUGO | S_IWUSR, lp5523_show_eng_start_addr, lp5523_store_eng_start_addr);
#endif // LEDS_LP5523_EXTENDED_FW

static DEVICE_ATTR(selftest, S_IRUGO, lp5523_selftest, NULL);

static struct attribute *lp5523_attributes[] = {
	&dev_attr_selftest.attr,
#if defined(CONFIG_LEDS_LP5523_PREDEFINED_PATTERNS)
	&dev_attr_led_pattern.attr,
#endif
#if defined(CONFIG_LEDS_LP5523_EXTENDED_FW)
	&dev_attr_eng_start_addr.attr,
#endif
	NULL,
};

static const struct attribute_group lp5523_group = {
	.attrs = lp5523_attributes,
};

/* Chip specific configurations */
static struct lp55xx_device_config lp5523_cfg = {
	.reset = {
		.addr = LP5523_REG_RESET,
		.val  = LP5523_RESET,
	},
	.enable = {
		.addr = LP5523_REG_ENABLE,
		.val  = LP5523_ENABLE,
	},
	.max_channel  = LP5523_MAX_LEDS,
#if !defined(CONFIG_LEDS_LP55XX_COMMON_DISABLE_RESET_AND_ENABLE)
	.post_init_device   = lp5523_post_init_device,
#endif
	.brightness_work_fn = lp5523_led_brightness_work,
	.set_led_current    = lp5523_set_led_current,
	.firmware_cb        = lp5523_firmware_loaded,
	.run_engine         = lp5523_run_engine,
	.dev_attr_group     = &lp5523_group,
};

static int lp5523_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	struct lp55xx_chip *chip;
	struct lp55xx_led *led;
	struct lp55xx_platform_data *pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "no platform data\n");
		return -EINVAL;
	}

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	led = devm_kzalloc(&client->dev,
			sizeof(*led) * pdata->num_channels, GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	chip->cl = client;
	chip->pdata = pdata;
	chip->cfg = &lp5523_cfg;

	mutex_init(&chip->lock);

	i2c_set_clientdata(client, led);

	ret = lp55xx_init_device(chip);
	if (ret)
		goto err_init;

	dev_info(&client->dev, "%s Programmable led chip found\n", id->name);

	ret = lp55xx_register_leds(led, chip);
	if (ret)
		goto err_register_leds;

	ret = lp55xx_register_sysfs(chip);
	if (ret) {
		dev_err(&client->dev, "registering sysfs failed\n");
		goto err_register_sysfs;
	}

	return 0;

err_register_sysfs:
	lp55xx_unregister_leds(led, chip);
err_register_leds:
	lp55xx_deinit_device(chip);
err_init:
	return ret;
}

static int lp5523_remove(struct i2c_client *client)
{
	struct lp55xx_led *led = i2c_get_clientdata(client);
	struct lp55xx_chip *chip = led->chip;

	lp5523_stop_engine(chip);
	lp55xx_unregister_sysfs(chip);
	lp55xx_unregister_leds(led, chip);
	lp55xx_deinit_device(chip);

	return 0;
}

static const struct i2c_device_id lp5523_id[] = {
	{ "lp5523",  LP5523 },
	{ "lp55231", LP55231 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, lp5523_id);

static struct i2c_driver lp5523_driver = {
	.driver = {
		.name	= "lp5523x",
	},
	.probe		= lp5523_probe,
	.remove		= lp5523_remove,
	.id_table	= lp5523_id,
};

module_i2c_driver(lp5523_driver);

MODULE_AUTHOR("Mathias Nyman <mathias.nyman@nokia.com>");
MODULE_AUTHOR("Milo Kim <milo.kim@ti.com>");
MODULE_DESCRIPTION("LP5523 LED engine");
MODULE_LICENSE("GPL");
