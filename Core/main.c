
#include "Core.h"
#include "GPIO.h"

#include "USB.h"


int main(void)
{
	CORE_Init();

	USB_Init();

	GPIO_EnableOutput(LED_TX_GPIO, LED_TX_PIN, false);
	GPIO_EnableOutput(LED_RX_GPIO, LED_RX_PIN, false);

	while(1)
	{
		CORE_Delay(1000);
		USB_CDC_WriteStr("Tick\r\n");

		//CORE_Idle();
	}
}

