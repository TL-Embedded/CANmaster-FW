
#include "MAX3301.h"
#include "GPIO.h"
#include "US.h"

/*
 * PRIVATE DEFINITIONS
 */

#define MAX3301_CODE_OVERCURRENT	0x2A
#define MAX3301_CODE_OVERVOLTAGE	0x2C
#define MAX3301_CODE_TXFAILURE		0x32

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

static bool MAX3301_ReadSync(uint32_t count);
static uint32_t MAX3301_ReadBits(uint32_t count);
static MAX3301_Fault_t MAX3301_DecodeFaultCode(uint32_t code);

/*
 * PRIVATE VARIABLES
 */

/*
 * PUBLIC FUNCTIONS
 */

void MAX3301_Init(void)
{
	GPIO_EnableInput(MAX3301_FAULT_PIN, GPIO_Pull_Up);
}

void MAX3301_Deinit(void)
{
	GPIO_Deinit(MAX3301_FAULT_PIN);
}

bool MAX3301_IsFaultSet(void)
{
	return GPIO_Read(MAX3301_FAULT_PIN);
}

MAX3301_Fault_t MAX3301_ClearFault(void)
{
	GPIO_EnableOutput(MAX3301_CANTX_PIN, GPIO_PIN_RESET);
	GPIO_EnableInput(MAX3301_CANRX_PIN, GPIO_Pull_None);

	// Sequence is:
	//    10 bit start sequence
	//    6 bit fault code
	//    10 bit clear sequence

	// However - we do not know how far into the start sequence we are
	// The top bit is always set - so we can read until we hit this.
	MAX3301_ReadSync(11);
	// Assume the top bit was successfully read.
	uint32_t code = MAX3301_ReadBits(5) | (1 << 5);
	// Discard the remaining bits.
	MAX3301_ReadBits(10);

	GPIO_Deinit(MAX3301_CANTX_PIN | MAX3301_CANRX_PIN);

	return MAX3301_DecodeFaultCode(code);
}

/*
 * PRIVATE FUNCTIONS
 */

static bool MAX3301_ReadSync(uint32_t count)
{
	// Read until we find a high bit.
	while (count--)
	{
		US_Delay(1);
		GPIO_Set(MAX3301_CANTX_PIN);
		US_Delay(1);
		bool bit = GPIO_Read(MAX3301_CANRX_PIN);
		GPIO_Reset(MAX3301_CANTX_PIN);

		if (bit) { return true; }
	}
	return false;
}

static uint32_t MAX3301_ReadBits(uint32_t count)
{
	// Read the specified number of bits.
	uint32_t value = 0;
	while (count--)
	{
		US_Delay(1);
		GPIO_Set(MAX3301_CANTX_PIN);
		US_Delay(1);
		value <<= 1;
		value |= GPIO_Read(MAX3301_CANRX_PIN) ? 1 : 0;
		GPIO_Reset(MAX3301_CANTX_PIN);
	}
	return value;
}

static MAX3301_Fault_t MAX3301_DecodeFaultCode(uint32_t code)
{
	switch (code)
	{
	case MAX3301_CODE_OVERCURRENT:
		return MAX3301_Fault_Overcurrent;
	case MAX3301_CODE_OVERVOLTAGE:
		return MAX3301_Fault_Overvoltage;
	case MAX3301_CODE_TXFAILURE:
		return MAX3301_Fault_TxFailure;
	default:
		return MAX3301_Fault_Unknown;
	}
}

/*
 * INTERRUPT ROUTINES
 */

