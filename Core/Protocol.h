#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "STM32X.h"
#include "CAN.h" // for message definitions only

/*
 * PUBLIC DEFINITIONS
 */

#define PROTCOL_CAN_ENCODE_MAX	16

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
	void (*transmit)(const CAN_Msg_t * msg);
	void (*configure)(const Protocol_Config_t * config);
	void (*get_status)(void);
} Protocol_Callback_t;

/*
 * PUBLIC FUNCTIONS
 */

void Protocol_Init(const Protocol_Callback_t * callback);
uint32_t Protocol_EncodeCan(const CAN_Msg_t * msg, uint8_t * bfr);
uint32_t Protocol_Decode(const uint8_t * data, uint32_t size);

/*
 * EXTERN DECLARATIONS
 */

#endif //PROTOCOL_H
