/******************************************************************************
 * @file    button.cpp
 * @author  Rémi Pincent - INRIA
 * @date    26 juin 2014   
 *
 * @brief button implementation
 * 
 * Project : button
 * Contact:  Rémi Pincent - remi.pincent@inria.fr
 * 
 * Revision History:
 * TODO_revision history
 *****************************************************************************/

#include "button.h"
#include "events.h"
#include <logger.h>
#include <EventManager.h>

/** default press durations  */
const uint16_t Button::DEFAULT_SHORT_PRESS_DURATION_MS = 250;
const uint16_t Button::DEFAULT_MEDIUM_PRESS_DURATION_MS = 5000;
const uint16_t Button::DEFAULT_LONG_PRESS_DURATION_MS = 10000;
const uint16_t Button::DEFAULT_LONG_LONG_PRESS_DURATION_MS = 15000;

/** indicates no press is on going => no timer on going */
const unsigned int Button::NO_COUNTING_DURATION_MS = 0xFFFF;
const unsigned int Button::PERIODIC_NOTIF_MS = 50;

Button::Button(unsigned int arg_u16_button_pin,
		uint8_t arg_u8_id,
		bool arg_b_pull_up) :
				_u8_button_pin(arg_u16_button_pin),
				_u16_current_press_dur(NO_COUNTING_DURATION_MS),
				_timer(this),
				_b_pull_up(arg_b_pull_up),
				_u8_id(arg_u8_id)
{
	/** Register as listener to process data in normal mode */
	EventManager::getInstance()->addListener( BUTTON_PRESSED_INT, this);
	EventManager::getInstance()->addListener( BUTTON_RELEASED_INT, this);

	/** configure button input */
	if(arg_b_pull_up)
	{
		pinMode(_u8_button_pin, INPUT_PULLUP);
	}
	else
	{
		pinMode(_u8_button_pin, INPUT_PULLDOWN);
	}
	/** Attach interrupt */
	if(arg_b_pull_up)
	{
		_e_lastEdge = FALLING;
	}
	else
	{
		_e_lastEdge = RISING;
	}
	attachInterrupt(_u8_button_pin, (ext_it_handler_t) (&buttonISR), _e_lastEdge, this);
}

Button::~Button()
{
	/** detach interrupt */
	detachInterrupt(_u8_button_pin);

	/** remove listener */
	EventManager::getInstance()->removeListener( this);
}

void Button::processEvent(uint8_t arg_u8_event_code, int arg_s16_event_param)
{
	LOG_VERBOSE_LN(F("Process event %d in button - param = %x"), arg_u8_event_code, arg_s16_event_param, this);

	/** get only my events */
	if((Button*) arg_s16_event_param != this)
	{
		return;
	}

	switch(arg_u8_event_code)
	{
	case BUTTON_PRESSED_INT :
		/** be sure consecutive presses are not get */
		if(_u16_current_press_dur == NO_COUNTING_DURATION_MS)
		{
		}
		else
		{
			/** cancel on going timer */
			if(_timer.isActive())
			{
				_timer.cancel();
			}
		}
		/** start timer to get press duration */
		_u16_current_press_dur = 0;
		_timer.periodicNotify(PERIODIC_NOTIF_MS);

		EventManager::getInstance()->queueEvent(BUTTON_PRESSED,
				(int) this,
				EventManager::kLowPriority);

		break;

	case BUTTON_RELEASED_INT :
		if(_timer.isActive())
		{
			_timer.cancel();
		}

		/** be sure consecutive releases are not get */
		if(_u16_current_press_dur != NO_COUNTING_DURATION_MS)
		{
			LOG_VERBOSE_LN(F("button release - press duration = %ums"), _u16_current_press_dur);
			if(_u16_current_press_dur <= DEFAULT_SHORT_PRESS_DURATION_MS)
			{
				EventManager::getInstance()->queueEvent(BUTTON_SHORT_PRESS,
						(int) this,
						EventManager::kLowPriority);
			}
			else if(_u16_current_press_dur >= DEFAULT_MEDIUM_PRESS_DURATION_MS
					&& _u16_current_press_dur <= DEFAULT_LONG_PRESS_DURATION_MS)
			{
				EventManager::getInstance()->queueEvent(BUTTON_MEDIUM_PRESS,
						(int) this,
						EventManager::kLowPriority);
			}
			else if(_u16_current_press_dur > DEFAULT_LONG_PRESS_DURATION_MS
					&& _u16_current_press_dur <= DEFAULT_LONG_LONG_PRESS_DURATION_MS)
			{
				EventManager::getInstance()->queueEvent(BUTTON_LONG_PRESS,
						(int) this,
						EventManager::kLowPriority);
			}
			else if(_u16_current_press_dur > DEFAULT_LONG_LONG_PRESS_DURATION_MS)
			{
				EventManager::getInstance()->queueEvent(BUTTON_LONG_LONG_PRESS,
						(int) this,
						EventManager::kLowPriority);
			}
			else
			{
				LOG_VERBOSE_LN(F("press of %d ms not handled by button"), _u16_current_press_dur);
				EventManager::getInstance()->queueEvent(BUTTON_RELEASED,
						(int) this,
						EventManager::kLowPriority);

			}
			_u16_current_press_dur = NO_COUNTING_DURATION_MS;
		}
		else
		{
			LOG_VERBOSE_LN(F("Cannot handle button release : no timer active - may be a rebound"));
		}
		break;

	default :
		/** should never be here */
		ASSERT(false);
		break;
	}
}

/**
 * button timer elapsed during button press
 */
void Button::timerElapsed(void)
{
	if(_u16_current_press_dur < DEFAULT_LONG_LONG_PRESS_DURATION_MS)
	{
		/** stop incrementation */
		_u16_current_press_dur += PERIODIC_NOTIF_MS;
	}
	else
	{
		/** no need to have more timer notifications */
		if(_timer.isActive())
		{
			_timer.cancel();
		}
	}
}

/**
 * CALLED UNDER INTERRUPT CONTEXT when some changes in button pin occur
 * @param arg_p_button_instance reference on instance using this button
 */
void Button::buttonISR(Button* arg_p_button_instance)
{
	byte loc_bLastItEdge = arg_p_button_instance->_e_lastEdge;

	bool loc_bState = digitalRead(arg_p_button_instance->_u8_button_pin);

	detachInterrupt(arg_p_button_instance->_u8_button_pin);

	if(loc_bState)
	{
		if(loc_bLastItEdge == RISING)
		{
			arg_p_button_instance->_e_lastEdge = FALLING;
			if(arg_p_button_instance->_b_pull_up)
			{
					EventManager::getInstance()->queueEvent( BUTTON_RELEASED_INT, (int) arg_p_button_instance );

			}
			else
			{
				EventManager::getInstance()->queueEvent( BUTTON_PRESSED_INT, (int) arg_p_button_instance );
			}
		}
		else
		{
			/** Spike detected */
		}
	}
	else
	{
		if(loc_bLastItEdge == FALLING)
		{
			arg_p_button_instance->_e_lastEdge = RISING;
			if(arg_p_button_instance->_b_pull_up)
			{
				EventManager::getInstance()->queueEvent( BUTTON_PRESSED_INT, (int) arg_p_button_instance );
			}
			else
			{
				EventManager::getInstance()->queueEvent( BUTTON_RELEASED_INT, (int) arg_p_button_instance );
			}
		}
		else
		{
			/** Spike detected */
		}
	}


	attachInterrupt(arg_p_button_instance->_u8_button_pin, (ext_it_handler_t) (&buttonISR), arg_p_button_instance->_e_lastEdge, arg_p_button_instance);
}
