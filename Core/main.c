
#include "Core.h"
#include "GPIO.h"
#include "USB.h"
#include "CAN.h"

#include "Protocol.h"
#include "Queue.h"

static Queue_t gCanTxQueue;
static CAN_Msg_t gCanTxBuffer[64];
static Protocol_Status_t gStatus = {0};


static void MAIN_TransmitCallback(const CAN_Msg_t * msg)
{
	if (!Queue_Push(&gCanTxQueue, msg))
	{
		gStatus.tx_errors += 1;
	}
}

static void MAIN_ConfigCallback(const Protocol_Config_t * config)
{
	CAN_Init(config->bitrate);
	CAN_EnableFilter(0, config->filter_id, config->filter_mask);
	GPIO_Write(CAN_TERM_GPIO, CAN_TERM_PIN, config->terminator);
}

static void MAIN_StatusCallback(Protocol_Status_t * status)
{
	*status = gStatus;
}

static const Protocol_Callback_t cProtocolCallbacks = {
	.tx_data = USB_CDC_Write,
	.rx_data = USB_CDC_Read,
	.configure = MAIN_ConfigCallback,
	.get_status = MAIN_StatusCallback,
	.tx_can = MAIN_TransmitCallback,
};

int main(void)
{
	CORE_Init();

	GPIO_EnableOutput(LED_TX_GPIO, LED_TX_PIN, false);
	GPIO_EnableOutput(LED_RX_GPIO, LED_RX_PIN, false);
	GPIO_EnableOutput(CAN_TERM_GPIO, CAN_TERM_PIN, false);
	GPIO_EnableOutput(CAN_MODE_GPIO, CAN_MODE_PIN, false);

	Queue_Init(&gCanTxQueue, gCanTxBuffer, sizeof(*gCanTxBuffer), LENGTH(gCanTxBuffer));

	Protocol_Config_t defaultConfig = {
		.bitrate = 250000,
		.filter_id = 0,
		.filter_mask = 0,
		.terminator = true,
	};
	MAIN_ConfigCallback(&defaultConfig);

	Protocol_Init(&cProtocolCallbacks);

	USB_Init();

	while(1)
	{
		// Read incoming can messages
		CAN_Msg_t rx;
		while (CAN_Read(&rx))
		{
			Protocol_RecieveCan(&rx);
		}

		CAN_Msg_t tx;
		if (   CAN_WriteFree() == CAN_MAILBOX_COUNT
			&& Queue_Pop(&gCanTxQueue, &tx))
		{
			// We only write in the first mailbox - to preserve message order.
			CAN_Write(&tx);
		}

		Protocol_Run();
	}
}

