#ifndef BOARD_H
#define BOARD_H

#define STM32F0

// Core config
//#define CORE_USE_TICK_IRQ

// CLK config
#define CLK_USE_HSE
#define CLK_HSE_FREQ		8000000
#define CLK_SYSCLK_FREQ		32000000

// CAN config
#define CAN_PINS			(PB8 | PB9)
#define CAN_AF				GPIO_AF4_CAN

// USB config
#define USB_ENABLE
#define USB_CLASS_CDC
#define USB_CDC_BFR_SIZE	512


// GPIO config
#define CAN_MODE_PIN		PC13
#define CAN_SNS_PIN			PC14
#define CAN_TERM_PIN		PA3

#define VERSION_PIN			PB12

#define LED_TX_PIN			PA4
#define LED_RX_PIN			PA15

#define GPIO_USE_IRQS

// MAX3301 config
#define MAX3301_FAULT_PIN	CAN_SNS_PIN
#define MAX3301_CANTX_PIN	PB9
#define MAX3301_CANRX_PIN	PB8


#endif /* BOARD_H */
