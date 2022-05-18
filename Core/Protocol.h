#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "STM32X.h"
#include "CAN.h" // for message definitions only

/*
 * PUBLIC DEFINITIONS
 */

/*
 * PUBLIC TYPES
 */

typedef struct {
	uint32_t filter_mask;
	uint32_t filter_id;
	bool terminator;
	uint32_t bitrate;
} Protocol_Config_t;

typedef struct {
	uint8_t tx_errors;
	uint8_t rx_errors;
} Protocol_Status_t;

typedef struct {
	void (*configure)(const Protocol_Config_t * config);
	void (*get_status)(Protocol_Status_t * status);

	void (*tx_can)(const CAN_Msg_t * msg);
	void (*tx_data)(const uint8_t * data, uint32_t len);
	uint32_t (*rx_data)(uint8_t * data, uint32_t max);

} Protocol_Callback_t;


/*
 * PUBLIC FUNCTIONS
 */

void Protocol_Init(const Protocol_Callback_t * callback);
void Protocol_Run(void);
void Protocol_RecieveCan(const CAN_Msg_t * msg);

/*
 * EXTERN DECLARATIONS
 */

#endif //PROTOCOL_H
