
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

/*
 * PRIVATE VARIABLES
 */

static Queue_t gCanTxQueue;
static CAN_Msg_t gCanTxBuffer[64];
static Protocol_Status_t gStatus = {0};
static Blinker_t gTxBlinker;
static Blinker_t gRxBlinker;

static const Protocol_Callback_t cProtocolCallbacks = {
	.tx_data = USB_CDC_Write,
	.rx_data = USB_CDC_Read,
	.configure = MAIN_ConfigCallback,
	.get_status = MAIN_StatusCallback,
	.tx_can = MAIN_TransmitCallback,
};

static Protocol_Config_t gProtocolConfig = {
	.bitrate = 250000,
	.filter_id = 0,
	.filter_mask = 0,
	.terminator = false,
};

/*
 * PUBLIC FUNCTIONS
 */

int main(void)
{
	CORE_Init();

	Blinker_Init(&gTxBlinker, LED_TX_GPIO, LED_TX_PIN);
	Blinker_Init(&gRxBlinker, LED_RX_GPIO, LED_RX_PIN);

	Blinker_Blink(&gTxBlinker, 500);
	Blinker_Blink(&gRxBlinker, 500);

	GPIO_EnableOutput(CAN_TERM_GPIO, CAN_TERM_PIN, false);
	GPIO_EnableOutput(CAN_MODE_GPIO, CAN_MODE_PIN, false);

	GPIO_EnableInput(VERSION_GPIO, VERSION_PIN, GPIO_Pull_Up);
	bool has_max3301 = !GPIO_Read(VERSION_GPIO, VERSION_PIN);

	if (has_max3301)
	{
		MAX3301_Init();
	}

	Queue_Init(&gCanTxQueue, gCanTxBuffer, sizeof(*gCanTxBuffer), LENGTH(gCanTxBuffer));
	MAIN_InitCAN(&gProtocolConfig);
	Protocol_Init(&cProtocolCallbacks);

	USB_Init();

	while(1)
	{
		if (has_max3301 && MAX3301_IsFaultSet())
		{
			CAN_Deinit();
			MAX3301_ClearFault();
			MAIN_InitCAN(&gProtocolConfig);
		}

		// Read incoming can messages
		CAN_Msg_t rx;
		while (CAN_Read(&rx))
		{
			Blinker_Blink(&gRxBlinker, 50);
			Protocol_RecieveCan(&rx);
		}

		CAN_Msg_t tx;
		if (   CAN_WriteFree() == CAN_MAILBOX_COUNT
			&& Queue_Pop(&gCanTxQueue, &tx))
		{
			Blinker_Blink(&gTxBlinker, 50);
			// We only write in the first mailbox - to preserve message order.
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
		gStatus.tx_errors += 1;
	}
}

static void MAIN_ConfigCallback(const Protocol_Config_t * config)
{
	gProtocolConfig = *config;
	MAIN_InitCAN(config);
}

static void MAIN_InitCAN(const Protocol_Config_t * config)
{
	CAN_Init(config->bitrate);
	CAN_EnableFilter(0, config->filter_id, config->filter_mask);
	GPIO_Write(CAN_TERM_GPIO, CAN_TERM_PIN, config->terminator);
}

static void MAIN_StatusCallback(Protocol_Status_t * status)
{
	*status = gStatus;
}

