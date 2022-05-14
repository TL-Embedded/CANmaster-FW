
#include "Protocol.h"

/*
 * PRIVATE DEFINITIONS
 */

#define PROTOCOL_CAN_EXT	(1 << 5)

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

static uint8_t Protocol_Checksum(const uint8_t * data, uint32_t count);
static uint32_t Protocol_GetBitrate(uint8_t code);

/*
 * PRIVATE VARIABLES
 */

static Protocol_Callback_t gProtocolCallback;

/*
 * PUBLIC FUNCTIONS
 */

void Protocol_Init(const Protocol_Callback_t * callback)
{
	gProtocolCallback = *callback;
}

uint32_t Protocol_EncodeCan(const CAN_Msg_t * msg, uint8_t * bfr)
{
	uint8_t * head = bfr;

	*head++ = 0xAA;

	if (msg->ext)
	{
		*head++ = 0xC0 | PROTOCOL_CAN_EXT | msg->len;

		*head++ = (msg->id >>  0);
		*head++ = (msg->id >>  8);
		*head++ = (msg->id >> 16);
		*head++ = (msg->id >> 24);
	}
	else
	{
		*head++ = 0xC0 | msg->len;

		*head++ = (msg->id >> 0);
		*head++ = (msg->id >> 8);
	}

	for (uint32_t i = 0; i < msg->len; i++)
	{
		*head++ = msg->data[i];
	}

	*head++ = 0x55;

	return head - bfr;
}

uint32_t Protocol_Decode(const uint8_t * data, uint32_t size)
{
	// This decoder runs optimally when the entire packet arrives at once
	// For USB traffic this is a normal case.

	if (size < 5)
	{
		// Insufficient for any packet type
		return 0;
	}

	if (data[0] != 0xAA)
	{
		// Packet does not start with start char.
		// Start consuming characters to find a start char.
		for (uint32_t i = 0; i < size; i++)
		{
			if (data[i] == 0xAA) { return i; }
		}
		return size;
	}

	if (data[1] == 0x55)
	{
		// full packet header
		if (data[2] == 0x12)
		{
			// configuration packet.
			uint32_t packet_size = 20;

			if (size < packet_size)
			{
				// No bytes consumed. Wait for a full packet
				return 0;
			}

			if (data[packet_size - 1] == Protocol_Checksum(&data[2], 17))
			{
				Protocol_Config_t config;

				config.bitrate = Protocol_GetBitrate(data[3]);
				config.filter_id =		  (data[ 5] <<  0)
										| (data[ 6] <<  8)
										| (data[ 7] << 16)
										| (data[ 8] << 24);

				config.filter_mask = 	  (data[ 9] <<  0)
									    | (data[10] <<  8)
									    | (data[11] << 16)
									    | (data[12] << 24);
				config.terminator = true;

				gProtocolCallback.configure(&config);
			}

			return packet_size;
		}
	}
	else if ((data[1] & 0xC0) == 0xC0)
	{
		// can packet
		CAN_Msg_t tx;
		tx.ext = data[1] & PROTOCOL_CAN_EXT;
		tx.len = data[1] & 0x0F;

		if (tx.len > 8)
		{
			// Not possible
			return 2;
		}

		uint32_t packet_size = 3 + (tx.ext ? 4 : 2) + tx.len;

		if (size < packet_size)
		{
			// No bytes consumed. Wait for a full packet
			return 0;
		}

		// Check for the stop character.
		// If its not present, the packet is still consumed
		if (data[packet_size - 1] == 0x55)
		{
			// Copy the actual fields out.
			if (tx.ext)
			{
				tx.id = (data[2] <<  0)
					  | (data[3] <<  8)
					  | (data[4] << 16)
					  | (data[5] << 24);
				memcpy(tx.data, &data[6], tx.len);
			}
			else
			{
				tx.id = (data[2] <<  0)
					  | (data[3] <<  8);
				memcpy(tx.data, &data[4], tx.len);
			}

			gProtocolCallback.transmit(&tx);
		}

		return packet_size;
	}

	// Header not handled by another case?
	return 2; // Discard the header.
}

/*
 * PRIVATE FUNCTIONS
 */

static uint32_t Protocol_GetBitrate(uint8_t code)
{
	switch (code)
	{
	case 0x01:
		return 1000000;
	case 0x02:
		return 800000;
	case 0x03:
		return 500000;
	case 0x04:
		return 400000;
	case 0x05:
		return 250000;
	case 0x06:
		return 200000;
	case 0x07:
		return 125000;
	case 0x08:
		return 100000;
	case 0x09:
		return 50000;
	case 0x0A:
		return 20000;
	case 0x0B:
		return 10000;
	case 0x0C:
	default:
		return 5000;
	}
}


static uint8_t Protocol_Checksum(const uint8_t * data, uint32_t count)
{
	uint32_t total = 0;
	while (count--)
	{
		total += *data++;
	}
	return total; // Intentional discarding of high bits.
}

/*
 * INTERRUPT ROUTINES
 */

