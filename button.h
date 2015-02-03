/******************************************************************************
 * @file    button.h
 * @author  Rémi Pincent - INRIA
 * @date    26 juin 2014   
 *
 * @brief button definition. Maps physical button push to software events
 * 
 * Project : button
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 * 
 * Revision History:
 * TODO_revision history
 *****************************************************************************/

#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>
#include <timer.h>
#include <EventListener.h>
#include "Arduino.h"

/**
 * @class Button
 * @brief button object detects short and long presses. It notifies it
 * over asynchronous events.
 *
 * Asynchronous events list :
 *   internally used :
 *      - BUTTON_PRESSED_INT
 *      - BUTTON_RELEASED_INT
 *   to be used externally :
 *      - BUTTON_PRESSED
 *      - BUTTON_RELEASED
 *      - BUTTON_SHORT_PRESS
 *      - BUTTON_LONG_PRESS
 * all those events must be defined in events.h
 */
class Button : public TimerListener, public EventListener
{
public :
	typedef enum{
		SHORT_PRESS = 0,
		MEDIUM_PRESS,
		LONG_PRESS,
		LONG_LONG_PRESS,
		UNDEFINED_PRESS
	}EPressType;

private :
	static const uint16_t DEFAULT_SHORT_PRESS_DURATION_MS;
	static const uint16_t DEFAULT_MEDIUM_PRESS_DURATION_MS;
	static const uint16_t DEFAULT_LONG_PRESS_DURATION_MS;
	static const uint16_t DEFAULT_LONG_LONG_PRESS_DURATION_MS;

	static const unsigned int NO_COUNTING_DURATION_MS;
	static const unsigned int PERIODIC_NOTIF_MS;
	uint8_t _u8_button_pin;
	unsigned int _u16_current_press_dur;
	/** to get press duration */
	Timer _timer;
	bool _b_pull_up;
	uint8_t _u8_id;
	/** spikes filtering */
	EPinTrigger _e_lastEdge;

public:
	Button(unsigned int arg_u16_button_pin,
			uint8_t arg_u8_id = 0,
			bool arg_b_pull_up = true);
	~Button();

	inline uint8_t getId(void)
	{
		return _u8_id;
	}

private :
	/**
	 * Asynchronous event occurred. Handle it.
	 * @param arg_u8_event_code
	 * @param arg_s16_event_param
	 */
	void processEvent(uint8_t arg_u8_event_code, int arg_s16_event_param);

	void timerElapsed(void);

	static void buttonISR(Button* arg_p_button_instance);
};

#endif /* BUTTON_H_ */
