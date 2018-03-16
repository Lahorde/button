/******************************************************************************
 * @file    button.h
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
#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C"
{
#endif
/**************************************************************************
 * Include Files
 **************************************************************************/
#include "exti_listener.h"
#include "stm32f0xx_hal_def.h"
#include "timer.h"
#include <stdint.h>

/**************************************************************************
 * Manifest Constants
 **************************************************************************/

/**************************************************************************
 * Type Definitions
 **************************************************************************/
typedef enum{
	SHORT_PRESS = 0,
	MEDIUM_PRESS,
	LONG_PRESS,
	LONG_LONG_PRESS,
	UNDEFINED_PRESS
}EPressType;

typedef enum{
	PULL_UP,
	PULL_DOWN,
	OUT_OF_ENUM_CONFIG
}EBtnConfig;

typedef void (*TButtonCb)(void*, EPressType, uint32_t);

typedef struct
{
	TButtonCb _pfn_cb;
	void* _p_cb_data;
	EBtnConfig _e_btnConfig;
	EEdge _e_lastEdge;
	GPIO_TypeDef* _ps_GPIOx;
	uint16_t _u16_gpioPin;
	TsTimer _s_timer;
	uint32_t _u32_pressStart;
}TsButton;

/**************************************************************************
 * Global variables
 **************************************************************************/

/**************************************************************************
 * Macros
 **************************************************************************/

/**************************************************************************
 * Global Functions Declarations
 **************************************************************************/
void Button_init(TsButton*);

void Button_start(TsButton* arg_ps_button);

void Button_stop(TsButton* arg_ps_button);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
