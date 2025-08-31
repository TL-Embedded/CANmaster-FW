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
	uint32_t bitrate;
	bool terminator;
	bool silent_mode;
	bool enable_errors;
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

typedef enum {
	Protocol_Error_Unknown 		= 0,
	Protocol_Error_Overcurrent 	= 1,
	Protocol_Error_Overvoltage 	= 2,
	Protocol_Error_TxFailure 	= 3,
	Protocol_Error_BufferFull 	= 4,

	// Note, this aligns with CAN_Error_t (+5)
	Protocol_Error_Stuff 			= 5,
	Protocol_Error_Form 			= 6,
	Protocol_Error_Acknowledgement 	= 7,
	Protocol_Error_RecessiveBit		= 8,
	Protocol_Error_DominantBit		= 9,
	Protocol_Error_CRC				= 10,
	Protocol_Error_Software			= 11,
	Protocol_Error_RxOverrun		= 12,

} Protocol_Error_t;


/*
 * PUBLIC FUNCTIONS
 */

void Protocol_Init(const Protocol_Callback_t * callback);
void Protocol_Run(void);
void Protocol_RecieveCan(const CAN_Msg_t * msg);
void Protocol_RecieveError(Protocol_Error_t error);

/*
 * EXTERN DECLARATIONS
 */

#endif //PROTOCOL_H
