#ifndef MAX3301_H
#define MAX3301_H

#include "STM32X.h"

/*
 * PUBLIC DEFINITIONS
 */

typedef enum {
	MAX3301_Fault_Unknown = 0,
	MAX3301_Fault_Overcurrent,
	MAX3301_Fault_Overvoltage,
	MAX3301_Fault_TxFailure,
} MAX3301_Fault_t;

/*
 * PUBLIC TYPES
 */

/*
 * PUBLIC FUNCTIONS
 */

void MAX3301_Init(void);
void MAX3301_Deinit(void);

bool MAX3301_IsFaultSet(void);
MAX3301_Fault_t MAX3301_ClearFault(void);

/*
 * EXTERN DECLARATIONS
 */

#endif //MAX3301_H
