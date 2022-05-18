
#include "Protocol.h"

/*
 * PRIVATE DEFINITIONS
 */

#define PROTOCOL_CAN_EXT		(1 << 5)

#define PROTOCOL_CAN_ENCODE_MAX		16
#define PROTOCOL_STATUS_ENCODE_MAX	20

/*
 * PRIVATE TYPES
 */

/*
 * PRIVATE PROTOTYPES
 */

static uint8_t Protocol_Checksum(const uint8_t * data, uint32_t count);
static uint32_t Protocol_GetBitrate(uint8_t code);
static uint32_t Protocol_EncodeCan(const CAN_Msg_t * msg, uint8_t * bfr);
static uint32_t Protocol_DecodeData(const uint8_t * data, uint32_t size);

/*
 * PRIVATE VARIABLES
 */

static Protocol_Callback_t gProtocolCallback;

static struct {
	uint32_t head;
	uint8_t buffer[128];
} gRx;

/*
 * PUBLIC FUNCTIONS
 */

void Protocol_Init(const Protocol_Callback_t * callback)
{
	gProtocolCallback = *callback;
	gRx.head = 0;
}

void Protocol_RecieveCan(const CAN_Msg_t * msg)
{
	uint8_t txbfr[PROTOCOL_CAN_ENCODE_MAX];
	uint32_t txlen = Protocol_EncodeCan(msg, txbfr);
	gProtocolCallback.tx_data(txbfr, txlen);
}

void Protocol_Run(void)
{
	// Read incoming USB data
	gRx.head += gProtocolCallback.rx_data(gRx.buffer + gRx.head, sizeof(gRx.buffer) - gRx.head);
	uint32_t rxtail = 0;

	// Try to process messages consecutively from the buffer
	while (rxtail < gRx.head)
	{
		uint32_t processed = Protocol_DecodeData(gRx.buffer + rxtail, gRx.head - rxtail);
		if (processed == 0)
		{
			// Stop if the protocol stops consuming bytes
			break;
		}
		rxtail += processed;
	}

	// Now deal with any remaining data.
	int32_t remaining = gRx.head - rxtail;
	if (remaining > 0)
	{
		// Copy this back to the start of the buffer.
		for (uint32_t i = 0; i < remaining; i++)
		{
			gRx.buffer[i] = gRx.buffer[i + rxtail];
		}
		gRx.head = remaining;
	}
	else
	{
		gRx.head = 0;
	}
}

/*
 * PRIVATE FUNCTIONS
 */


static uint32_t Protocol_EncodeCan(const CAN_Msg_t * msg, uint8_t * bfr)
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

static uint32_t Protocol_EncodeStatus(const Protocol_Status_t * status, uint8_t * bfr)
{
	uint8_t * head = bfr;

	*head++ = 0xAA;
	*head++ = 0x55;
	*head++ = 0x04;
	*head++ = status->rx_errors;
	*head++ = status->tx_errors;

	for (uint32_t i = 0; i < 14; i++)
	{
		*head++ = 0;
	}

	*head++ = Protocol_Checksum(&bfr[2], 17);

	return head - bfr;
}

static uint32_t Protocol_DecodeData(const uint8_t * data, uint32_t size)
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
				gProtocolCallback.tx_data(data, packet_size);
			}

			return packet_size;
		}
		else if (data[2] == 0x04)
		{
			// Status request
			uint32_t packet_size = 20;

			if (size < packet_size)
			{
				// No bytes consumed. Wait for a full packet
				return 0;
			}

			if (data[packet_size - 1] == Protocol_Checksum(&data[2], 17))
			{
				Protocol_Status_t status;
				gProtocolCallback.get_status(&status);
				uint8_t bfr[PROTOCOL_STATUS_ENCODE_MAX];
				uint32_t len = Protocol_EncodeStatus(&status, bfr);
				gProtocolCallback.tx_data(bfr, len);
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

			gProtocolCallback.tx_can(&tx);
		}

		return packet_size;
	}

	// Header not handled by another case?
	return 2; // Discard the header.
}


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

