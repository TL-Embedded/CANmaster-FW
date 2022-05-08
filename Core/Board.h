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
#define CAN_GPIO			GPIOB
#define CAN_PINS			(GPIO_PIN_8 | GPIO_PIN_9)
#define CAN_AF				GPIO_AF4_CAN

// USB config
#define USB_ENABLE
#define USB_CLASS_CDC
#define USB_CDC_BFR_SIZE	512


// GPIO config
#define CAN_MODE_GPIO		GPIOC
#define CAN_MODE_PIN		GPIO_PIN_13
#define CAN_TERM_GPIO		GPIOA
#define CAN_TERM_PIN		GPIO_PIN_3

#define LED_TX_GPIO			GPIOA
#define LED_TX_PIN			GPIO_PIN_4
#define LED_RX_GPIO			GPIOA
#define LED_RX_PIN			GPIO_PIN_15

#endif /* BOARD_H */
