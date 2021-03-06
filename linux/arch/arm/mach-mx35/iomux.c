/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @defgroup GPIO_MX35 Board GPIO and Muxing Setup
 * @ingroup MSL_MX35
 */
/*!
 * @file mach-mx35/iomux.c
 *
 * @brief I/O Muxing control functions
 *
 * @ingroup GPIO_MX35
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>
#include "iomux.h"

/*!
 * IOMUX register (base) addresses
 */
enum iomux_reg_addr {
	IOMUXGPR = IO_ADDRESS(IOMUXC_BASE_ADDR),
	/*!< General purpose */
	IOMUXSW_MUX_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + 4,
	/*!< MUX control */
	IOMUXSW_MUX_END = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x324,
	/*!< last MUX control register */
	IOMUXSW_PAD_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x328,
	/*!< Pad control */
	IOMUXSW_PAD_END = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x790,
	/*!< last Pad control register */
	IOMUXSW_INPUT_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x7A8,
	/*!< input select register */
	IOMUXSW_INPUT_END = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x9F4,
	/*!< last input select register */
};

#define MUX_PIN_NUM_MAX		\
		(((IOMUXSW_PAD_END - IOMUXSW_PAD_CTL) >> 2) + 1)
#define MUX_INPUT_NUM_MUX	\
		(((IOMUXSW_INPUT_END - IOMUXSW_INPUT_CTL) >> 2) + 1)

#define PIN_TO_IOMUX_INDEX(pin) ((PIN_TO_IOMUX_PAD(pin) - 0x328) >> 2)

static DEFINE_SPINLOCK(gpio_mux_lock);
static u8 iomux_pin_res_table[MUX_PIN_NUM_MAX];

/*!
 * This function is used to configure a pin through the IOMUX module.
 * FIXED ME: for backward compatible. Will be static function!
 * @param  pin		a pin number as defined in \b #iomux_pin_name_t
 * @param  cfg		an output function as defined in \b #iomux_pin_cfg_t
 *
 * @return 		0 if successful; Non-zero otherwise
 */
static int iomux_config_mux(iomux_pin_name_t pin, iomux_pin_cfg_t cfg)
{
	u32 ret = 0;
	u32 pin_index = PIN_TO_IOMUX_INDEX(pin);
	u32 mux_reg = PIN_TO_IOMUX_MUX(pin);
	u8 *rp;

	BUG_ON(pin_index > MUX_PIN_NUM_MAX);
	if (mux_reg != NON_MUX_I) {
		mux_reg += IOMUXGPR;
		BUG_ON((mux_reg > IOMUXSW_MUX_END)
		       || (mux_reg < IOMUXSW_MUX_CTL));
		spin_lock(&gpio_mux_lock);
		__raw_writel(cfg, mux_reg);
		/*
		 * Log a warning if a pin changes ownership
		 */
		rp = iomux_pin_res_table + pin_index;
		if ((cfg & *rp) && (*rp != cfg)) {
			/*Console: how to do */
			printk(KERN_ERR "iomux_config_mux: Warning: iomux pin"
			       " config changed, index=%d register=%d, "
			       " prev=0x%x new=0x%x\n", pin_index, mux_reg,
			       *rp, cfg);
			ret = -EINVAL;
		}
		*rp = cfg;
		spin_unlock(&gpio_mux_lock);
	}

	return ret;
}

/*!
 * Request ownership for an IO pin. This function has to be the first one
 * being called before that pin is used. The caller has to check the
 * return value to make sure it returns 0.
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  cfg		an input function as defined in \b #iomux_pin_cfg_t
 *
 * @return		0 if successful; Non-zero otherwise
 */
int mxc_request_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t cfg)
{
	int ret = iomux_config_mux(pin, cfg);
	if (GPIO_TO_PORT(IOMUX_TO_GPIO(pin)) < GPIO_PORT_NUM) {
		if (((cfg & (~MUX_CONFIG_SION)) == MUX_CONFIG_GPIO) ||
		    (((cfg & (~MUX_CONFIG_SION)) == MUX_CONFIG_FUNC) &&
		     ((pin == MX35_PIN_GPIO1_0) || (pin == MX35_PIN_GPIO1_1) ||
		      (pin == MX35_PIN_GPIO2_0) || (pin == MX35_PIN_GPIO3_0))))
			ret |= mxc_request_gpio(pin);
	}
	return ret;
}

EXPORT_SYMBOL(mxc_request_iomux);

/*!
 * Release ownership for an IO pin
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  cfg		an input function as defined in \b #iomux_pin_cfg_t
 */
void mxc_free_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t cfg)
{
	u32 pin_index = PIN_TO_IOMUX_INDEX(pin);
	u8 *rp = iomux_pin_res_table + pin_index;

	BUG_ON((pin_index > MUX_PIN_NUM_MAX));

	*rp = 0;
	if (GPIO_TO_PORT(IOMUX_TO_GPIO(pin)) < GPIO_PORT_NUM) {
		if (((cfg & (~MUX_CONFIG_SION)) == MUX_CONFIG_GPIO) ||
		    (((cfg & (~MUX_CONFIG_SION)) == MUX_CONFIG_FUNC) &&
		     ((pin == MX35_PIN_GPIO1_0) || (pin == MX35_PIN_GPIO1_1) ||
		      (pin == MX35_PIN_GPIO2_0) || (pin == MX35_PIN_GPIO3_0))))
			mxc_free_gpio(pin);
	}
}

EXPORT_SYMBOL(mxc_free_iomux);

/*!
 * This function configures the pad value for a IOMUX pin.
 *
 * @param  pin     a pin number as defined in \b #iomux_pin_name_t
 * @param  config  the ORed value of elements defined in \b #iomux_pad_config_t
 */
void mxc_iomux_set_pad(iomux_pin_name_t pin, u32 config)
{
	u32 pad_reg = IOMUXGPR + PIN_TO_IOMUX_PAD(pin);

	BUG_ON((pad_reg > IOMUXSW_PAD_END) || (pad_reg < IOMUXSW_PAD_CTL));

	spin_lock(&gpio_mux_lock);
	__raw_writel(config, pad_reg);
	spin_unlock(&gpio_mux_lock);
}

EXPORT_SYMBOL(mxc_iomux_set_pad);

/*!
 * This function enables/disables the general purpose function for a particular
 * signal.
 *
 * @param  gp   one signal as defined in \b #iomux_gp_func_t
 * @param  en   \b #true to enable; \b #false to disable
 */
void mxc_iomux_set_gpr(iomux_gp_func_t gp, bool en)
{
	u32 l;

	spin_lock(&gpio_mux_lock);
	l = __raw_readl(IOMUXGPR);

	if (en)
		l |= gp;
	else
		l &= ~gp;

	__raw_writel(l, IOMUXGPR);
	spin_unlock(&gpio_mux_lock);
}

EXPORT_SYMBOL(mxc_iomux_set_gpr);

/*!
 * This function configures input path.
 *
 * @param input index of input select register as defined in \b
 *  			#iomux_input_select_t
 * @param config the binary value of elements defined in \b
 * 			#iomux_input_config_t
 */
void mxc_iomux_set_input(iomux_input_select_t input, u32 config)
{
	u32 reg = IOMUXSW_INPUT_CTL + (input << 2);

	BUG_ON(input >= MUX_INPUT_NUM_MUX);

	spin_lock(&gpio_mux_lock);
	__raw_writel(config, reg);
	spin_unlock(&gpio_mux_lock);
}

EXPORT_SYMBOL(mxc_iomux_set_input);
