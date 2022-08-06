
#include "Blinker.h"
#include "Core.h"

/*
 * PRIVATE DEFINITIONS
 */

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

/*
 * PRIVATE VARIABLES
 */

/*
 * PUBLIC FUNCTIONS
 */

void Blinker_Init(Blinker_t * b, GPIO_Pin_t pin)
{
	b->pin = pin;
	b->is_on = false;
	GPIO_EnableOutput(b->pin, false);
}

void Blinker_Update(Blinker_t * b)
{
	if (b->is_on)
	{
		int32_t remaining = b->timeout - CORE_GetTick();
		if (remaining <= 0)
		{
			b->is_on = false;
			GPIO_Reset(b->pin);
		}
	}
}

void Blinker_Blink(Blinker_t * b, uint32_t timeout)
{
	b->is_on = !b->is_on;
	GPIO_Write(b->pin, b->is_on);
	b->timeout = CORE_GetTick() + timeout;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * INTERRUPT ROUTINES
 */

