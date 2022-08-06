#ifndef BLINKER_H
#define BLINKER_H

#include "STM32X.h"
#include "GPIO.h"

/*
 * PUBLIC DEFINITIONS
 */

/*
 * PUBLIC TYPES
 */

typedef struct {
	GPIO_Pin_t pin;
	uint32_t timeout;
	bool is_on;
} Blinker_t;

/*
 * PUBLIC FUNCTIONS
 */

void Blinker_Init(Blinker_t * b, GPIO_Pin_t pin);
void Blinker_Update(Blinker_t * b);
void Blinker_Blink(Blinker_t * b, uint32_t timeout);

/*
 * EXTERN DECLARATIONS
 */

#endif //BLINKER_H
