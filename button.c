/******************************************************************************
 * @file    button.c
 * @author  Rémi Pincent - INRIA
 * @date    01/03/2018
 *
 * @brief Handle button with different press durations.
 *
 * Project : peopeo
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 *
 * Revision History:
 * Insert github reference
 *
 * LICENSE :
 * peopeo (c) by Rémi Pincent
 * peopeo is licensed under a
 * Creative Commons Attribution-NonCommercial 3.0 Unported License.
 *
 * You should have received a copy of the license along with this
 * work.  If not, see <http://creativecommons.org/licenses/by-nc/3.0/>.
 *****************************************************************************/

/**************************************************************************
 * Include Files
 **************************************************************************/
#include "button.h"
#include "exti_listener.h"
#include "timer.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal.h"
#include "assert.h"
#include <stddef.h>
#include <stdbool.h>

/**************************************************************************
 * Manifest Constants
 **************************************************************************/
#define DEBOUNCE_DUR (5U)

#define DEFAULT_SHORT_PRESS_DURATION_MS     (200U)
#define DEFAULT_MEDIUM_PRESS_DURATION_MS    (700U)
#define DEFAULT_LONG_PRESS_DURATION_MS      (2000U)
#define DEFAULT_LONG_LONG_PRESS_DURATION_MS (6000U)

/**************************************************************************
 * Type Definitions
 **************************************************************************/

/**************************************************************************
 * Global Variables
 **************************************************************************/

/**************************************************************************
 * Static Variables
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/

/**************************************************************************
 * Local Functions Declarations
 **************************************************************************/
static void Button_cbIT(TsButton*);
static void Button_checkLevel(TsButton*);
static bool Button_checkInput(TsButton*, bool);
static void Button_debounceTimerElapsed(TsButton*);
static EPressType Button_getPressType(uint32_t);
/**************************************************************************
 * Global Functions Definitions
 **************************************************************************/
void Button_init(TsButton* arg_ps_button)
{
	assert(arg_ps_button != NULL
			&& arg_ps_button->_e_btnConfig < OUT_OF_ENUM_CONFIG
			&& arg_ps_button->_pfn_cb != NULL
			&& arg_ps_button->_ps_GPIOx != NULL);

	arg_ps_button->_e_lastEdge = OUT_OF_ENUM_EDGE;

	arg_ps_button->_s_timer._pfn_cb = (void (*)(void*))&Button_debounceTimerElapsed;
	arg_ps_button->_s_timer._p_cbParam = arg_ps_button;
	Timer_init(&arg_ps_button->_s_timer);

	arg_ps_button->_u32_pressStart = 0;
}

void Button_start(TsButton* arg_ps_button)
{
	assert(arg_ps_button != NULL
			&& arg_ps_button->_e_btnConfig  < OUT_OF_ENUM_CONFIG
			&& arg_ps_button->_pfn_cb != NULL
			&& arg_ps_button->_ps_GPIOx != NULL);

	TsExtIListener loc_s_extListener = {
			._pfn_cb = (TExtIListenerFn) &Button_cbIT,
			._p_cbData = arg_ps_button,
			._u16_gpioPin = arg_ps_button->_u16_gpioPin
	};

	arg_ps_button->_e_lastEdge = (Button_checkInput(arg_ps_button, HAL_GPIO_ReadPin(arg_ps_button->_ps_GPIOx, arg_ps_button->_u16_gpioPin))) ? RISING : FALLING;

	if((arg_ps_button->_e_btnConfig == PULL_UP && arg_ps_button->_e_lastEdge == FALLING)
			|| (arg_ps_button->_e_btnConfig == PULL_DOWN && arg_ps_button->_e_lastEdge == RISING))
	{
		arg_ps_button->_u32_pressStart = HAL_GetTick();
		Timer_notifyAfter(&arg_ps_button->_s_timer, DEBOUNCE_DUR, arg_ps_button);
	}
	ExtIListener_registerListener(&loc_s_extListener);
}

void Button_stop(TsButton* arg_ps_button)
{

}

 /**************************************************************************
 * Local Functions Definitions
 **************************************************************************/
void Button_cbIT(TsButton* arg_ps_button)
{
	assert(arg_ps_button != NULL
			&& arg_ps_button->_e_btnConfig  < OUT_OF_ENUM_CONFIG
			&& arg_ps_button->_pfn_cb != NULL
			&& arg_ps_button->_ps_GPIOx != NULL);

	bool loc_b_inputVal = Button_checkInput(arg_ps_button, HAL_GPIO_ReadPin(arg_ps_button->_ps_GPIOx, arg_ps_button->_u16_gpioPin));
	EEdge loc_e_edge = (loc_b_inputVal) ? RISING : FALLING;

	/** check no edges have been missed */
	if(loc_e_edge == arg_ps_button->_e_lastEdge)
	{
		return;
	}

	arg_ps_button->_e_lastEdge =loc_e_edge;

	/** is a button press currently detected */
	if(Timer_isActive(&arg_ps_button->_s_timer))
	{
		/** an edge has been detected */
		Timer_cancel(&arg_ps_button->_s_timer);
		arg_ps_button->_u32_pressStart = 0;
	}
	else
	{
		if((arg_ps_button->_e_btnConfig == PULL_UP && loc_e_edge == RISING)
				|| (arg_ps_button->_e_btnConfig == PULL_DOWN && loc_e_edge == FALLING))
		{
			/** button released */
			arg_ps_button->_u32_pressStart = HAL_GetTick() - arg_ps_button->_u32_pressStart;
			arg_ps_button->_pfn_cb(arg_ps_button->_p_cb_data, Button_getPressType(arg_ps_button->_u32_pressStart), arg_ps_button->_u32_pressStart);
		}
		else if((arg_ps_button->_e_btnConfig == PULL_UP && loc_e_edge == FALLING)
			|| (arg_ps_button->_e_btnConfig == PULL_DOWN && loc_e_edge == RISING))
		{
			/** button pressed */
			arg_ps_button->_u32_pressStart = HAL_GetTick();
			Timer_notifyAfter(&arg_ps_button->_s_timer, DEBOUNCE_DUR, arg_ps_button);
		}
	}
	arg_ps_button->_e_lastEdge = loc_e_edge;
}

bool Button_checkInput(TsButton* arg_ps_button, bool arg_b_val)
{
	assert(arg_ps_button != NULL
			&& arg_ps_button->_e_btnConfig < OUT_OF_ENUM_CONFIG
			&& arg_ps_button->_pfn_cb != NULL
			&& arg_ps_button->_ps_GPIOx != NULL);

	for(uint8_t i = 0; i < 5; i++)
	{
		if(HAL_GPIO_ReadPin(arg_ps_button->_ps_GPIOx, arg_ps_button->_u16_gpioPin) != arg_b_val)
		{
			return !arg_b_val;
		}
	}
	return arg_b_val;
}

void Button_debounceTimerElapsed(TsButton* arg_ps_button)
{
	/** Nothing to do */
}

EPressType Button_getPressType(uint32_t arg_u32_dur)
{
	if(arg_u32_dur < DEFAULT_SHORT_PRESS_DURATION_MS)
	{
		return SHORT_PRESS;
	}
	else if(arg_u32_dur >= DEFAULT_SHORT_PRESS_DURATION_MS && arg_u32_dur < DEFAULT_MEDIUM_PRESS_DURATION_MS)
	{
		return MEDIUM_PRESS;
	}
	else if(arg_u32_dur >= DEFAULT_MEDIUM_PRESS_DURATION_MS && arg_u32_dur < DEFAULT_LONG_PRESS_DURATION_MS)
	{
		return LONG_PRESS;
	}
	else if(arg_u32_dur >= DEFAULT_LONG_PRESS_DURATION_MS && arg_u32_dur < DEFAULT_LONG_LONG_PRESS_DURATION_MS)
	{
		return LONG_LONG_PRESS;
	}
	else
	{
		return UNDEFINED_PRESS;
	}
}
