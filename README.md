# CANmaster FW

This is the firmware for the CANmaster v1.1 hardware.

The CANmaster is a USB to CAN adaptor.

Features:
 * Configurable CAN bitrate
 * Software enableable 120R terminator
 * Configurable recieve filters
 * Read and writes CAN messages
 * Enumerates as a standard USB serial port on windows and linux without additional drivers
 * LED feedback for transmit and recieve

# Build and programming
This firmware was build using STM32CubeIDE v1.8.0.

The firmware is loaded over SWD via the 6 pin TAG connect port

# Protocol

An example driver in python is available here: [canmaster.py](./Tests/canmaster.py)

The data is sent in a binary format. The configured baud rate of the serial port is unimportant. All messages start with `0xAA` as a delimiter. The second byte can be used to determine the message type. Messages end with `0x55`.

## Configuration
The settings can be changed using the [configuration message](#configuration-message).

On boot, the default settings are:
| Setting      | Default                   |
|--------------|---------------------------|
| Bitrate      | 250000                    |
| Terminator   | Disabled                  |
| Filter ID    | 0x00000000                |
| Filter Mask  | 0x00000000                |

## Recieving messages:
When messages are recieved, they will be immediately forwarded over USB using either the [standard CAN message](#standard-can-message) or [extended CAN message](#extended-can-message).

## Transmitting messages:
Messages can be enqueued using the [standard CAN message](#standard-can-message) or [extended CAN message](#extended-can-message). Once enqueued, they will be transmitted in order. They will be automatically repeated until transmit success.

The transmit queue is 64 messages long. Exceeding this limit will cause messages to be dropped.

# Message definitions

## Standard CAN message
| Byte         | Data                      |
|--------------|---------------------------|
|  0           | 0xAA                      |
|  1, bit 7:4  | 0xC                       |
|  1, bit 0:3  | DLC. This must be 0 to 8  |
|  2           | Arbitration ID  0:7       |
|  3           | Arbitration ID  8:15      |
|  4 : 4 + DLC | data                      |
|  5 + DLC     | 0x55                      |

## Extended CAN message
| Byte         | Data                      |
|--------------|---------------------------|
|  0           | 0xAA                      |
|  1, bit 7:4  |        0xD.               |
|  1, bit 0:3  | DLC. This must be 0 to 8  |
|  2           | Arbitration ID  0:7       |
|  3           | Arbitration ID  8:15      |
|  4           | Arbitration ID  16:23     |
|  5           | Arbitration ID  24:31     |
|  6 : 6 + DLC | data                      |
|  7 + DLC     | 0x55                      |

## Configuration message:
| Byte        | Data                      |
|-------------|---------------------------|
|  0          | 0xAA                      |
|  1          | 0x13                      |
|  2, bit 7:1 |  0x00                     |
|  2, bit 0   | Terminator (1 = enabled)  |
|  3          | CAN Bitrate     0:7       |
|  4          | CAN Bitrate     8:15      |
|  5          | CAN Bitrate     16:23     |
|  6          | CAN Bitrate     23:31     |
|  7          | Filter ID       0:7       |
|  8          | Filter ID       8:15      |
|  9          | Filter ID       16:23     |
|  10         | Filter ID       23:31     |
|  11         | Filter Mask     8:15      |
|  12         | Filter Mask     16:23     |
|  13         | Filter Mask     23:31     |
|  14         | Filter Mask     23:31     |
|  15         | 0x55                      |