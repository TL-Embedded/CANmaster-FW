
#include "Core.h"
#include "GPIO.h"
#include "USB.h"
#include "CAN.h"

#include "Protocol.h"
#include "Queue.h"
#include "Blinker.h"
#include "MAX3301.h"


/*
 * PRIVATE DEFINITIONS
 */

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */
static void MAIN_InitCAN(const Protocol_Config_t * config);

static void MAIN_ConfigCallback(const Protocol_Config_t * config);
static void MAIN_TransmitCallback(const CAN_Msg_t * msg);
static void MAIN_StatusCallback(Protocol_Status_t * status);

static Protocol_Error_t MAIN_MAX3301FaultToError(MAX3301_Fault_t fault);

/*
 * PRIVATE VARIABLES
 */

static Queue_t gCanTxQueue;
static CAN_Msg_t gCanTxBuffer[64];
static Protocol_Status_t gStatus = {0};
static Blinker_t gTxBlinker;
static Blinker_t gRxBlinker;
static CAN_Error_t gCanError = CAN_Error_None;

static const Protocol_Callback_t cProtocolCallbacks = {
	.tx_data = USB_CDC_Write,
	.rx_data = USB_CDC_Read,
	.configure = MAIN_ConfigCallback,
	.get_status = MAIN_StatusCallback,
	.tx_can = MAIN_TransmitCallback,
};

static Protocol_Config_t gDefaultConfig = {
	.bitrate = 250000,
	.filter_id = 0,
	.filter_mask = 0,
	.terminator = false,
	.silent_mode = false,
	.enable_errors = false,
};


/*
 * PUBLIC FUNCTIONS
 */

int main(void)
{
	CORE_Init();

	Blinker_Init(&gTxBlinker, LED_TX_PIN);
	Blinker_Init(&gRxBlinker, LED_RX_PIN);

	GPIO_EnableOutput(CAN_TERM_PIN, false);
	GPIO_EnableOutput(CAN_MODE_PIN, false);

	// Version detection.
	GPIO_EnableInput(VERSION_PIN, GPIO_Pull_Up);
	bool has_max3301 = !GPIO_Read(VERSION_PIN);

	// Init parts & modules.
	if (has_max3301)
	{
		MAX3301_Init();
	}

	Queue_Init(&gCanTxQueue, gCanTxBuffer, sizeof(*gCanTxBuffer), LENGTH(gCanTxBuffer));
	MAIN_InitCAN(&gDefaultConfig);
	Protocol_Init(&cProtocolCallbacks);
	USB_Init();

	// Trigger our startup blink.
	Blinker_Blink(&gTxBlinker, 500);
	Blinker_Blink(&gRxBlinker, 500);

	while(1)
	{
		// Check for the fault signal from applicable transcievers.
		if (has_max3301 && MAX3301_IsFaultSet())
		{
			// MAX3301 signals through the RX & TX lines
			CAN_Deinit();
			MAX3301_Fault_t fault = MAX3301_ClearFault();
			Protocol_RecieveError(MAIN_MAX3301FaultToError(fault));
			MAIN_InitCAN(&gDefaultConfig);
		}

		if (gCanError != CAN_Error_None)
		{
			Protocol_RecieveError(gCanError + Protocol_Error_Stuff - 1);
			gCanError = CAN_Error_None;
		}

		// Read incoming can messages
		CAN_Msg_t rx;
		while (CAN_Read(&rx))
		{
			Blinker_Blink(&gRxBlinker, 50);
			Protocol_RecieveCan(&rx);
		}

		// Write outgoing can messages
		CAN_Msg_t tx;
		if (CAN_WriteFree() && Queue_Pop(&gCanTxQueue, &tx))
		{
			Blinker_Blink(&gTxBlinker, 50);
			CAN_Write(&tx);
		}

		Protocol_Run();
		Blinker_Update(&gRxBlinker);
		Blinker_Update(&gTxBlinker);
	}
}

/*
 * PRIVATE FUNCTIONS
 */

static void MAIN_TransmitCallback(const CAN_Msg_t * msg)
{
	if (!Queue_Push(&gCanTxQueue, msg))
	{
		Protocol_RecieveError(Protocol_Error_BufferFull);
		gStatus.tx_errors += 1;
	}
}

static void MAIN_ConfigCallback(const Protocol_Config_t * config)
{
	// Save the config in case we need to re-init
	gDefaultConfig = *config;
	MAIN_InitCAN(config);
}

static void MAIN_CanErrorCallback(CAN_Error_t error)
{
	gCanError = error;
}

static void MAIN_InitCAN(const Protocol_Config_t * config)
{
	// Running the mailbox in FIFO guarantees message TX order.
	CAN_Mode_t mode = CAN_Mode_TransmitFIFO;
	if (config->silent_mode) { mode |= CAN_Mode_Silent; }
	CAN_Init(config->bitrate, mode);
	CAN_EnableFilter(0, config->filter_id, config->filter_mask);
	GPIO_Write(CAN_TERM_PIN, config->terminator);
	CAN_OnError(MAIN_CanErrorCallback);
}

static void MAIN_StatusCallback(Protocol_Status_t * status)
{
	*status = gStatus;
}

static Protocol_Error_t MAIN_MAX3301FaultToError(MAX3301_Fault_t fault)
{
	switch (fault)
	{
	case MAX3301_Fault_Overcurrent:
		return Protocol_Error_Overcurrent;
	case MAX3301_Fault_Overvoltage:
		return Protocol_Error_Overvoltage;
	case MAX3301_Fault_TxFailure:
		return Protocol_Error_TxFailure;
	default:
	case MAX3301_Fault_Unknown:
		return Protocol_Error_Unknown;
	}
}

