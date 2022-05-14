
#include "Core.h"
#include "GPIO.h"
#include "USB.h"
#include "CAN.h"

#include "Protocol.h"
#include "Queue.h"

static Queue_t gCanTxQueue;
static CAN_Msg_t gCanTxBuffer[64];

static void MAIN_TransmitCallback(const CAN_Msg_t * msg)
{
	Queue_Push(&gCanTxQueue, msg);
}

static void MAIN_ConfigCallback(const Protocol_Config_t * config)
{
	CAN_Init(config->bitrate);
	CAN_EnableFilter(0, config->filter_id, config->filter_mask);
	GPIO_Write(CAN_TERM_GPIO, CAN_TERM_PIN, config->terminator);
}

static void MAIN_GetStatusCallback(void)
{

}

static const Protocol_Callback_t cProtocolCallbacks = {
	.transmit = MAIN_TransmitCallback,
	.configure = MAIN_ConfigCallback,
	.get_status = MAIN_GetStatusCallback,
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

	static uint8_t rxbfr[128];
	static uint32_t rxhead = 0;

	while(1)
	{
		// Read incoming can messages
		CAN_Msg_t rx;
		while (CAN_Read(&rx))
		{
			uint8_t txbfr[PROTCOL_CAN_ENCODE_MAX];
			uint32_t txlen = Protocol_EncodeCan(&rx, txbfr);
			USB_CDC_Write(txbfr, txlen);
		}

		CAN_Msg_t tx;
		if (   CAN_WriteFree() == CAN_MAILBOX_COUNT
			&& Queue_Pop(&gCanTxQueue, &tx))
		{
			// We only write in the first mailbox - to preserve message order.

			CAN_Write(&tx);
		}

		// Read incoming USB data
		rxhead += USB_CDC_Read(rxbfr + rxhead, sizeof(rxbfr) - rxhead);
		uint32_t rxtail = 0;

		// Try to process messages consecutively from the buffer
		while (rxtail < rxhead)
		{
			uint32_t processed = Protocol_Decode(rxbfr + rxtail, rxhead - rxtail);
			if (processed == 0)
			{
				// Stop if the protocol stops consuming bytes
				break;
			}
			rxtail += processed;
		}

		// Now deal with any remaining data.
		int32_t remaining = rxhead - rxtail;
		if (remaining > 0)
		{
			// Copy this back to the start of the buffer.
			for (uint32_t i = 0; i < remaining; i++)
			{
				rxbfr[i] = rxbfr[i + rxtail];
			}
			rxhead = remaining;
		}
		else
		{
			rxhead = 0;
		}
	}
}

