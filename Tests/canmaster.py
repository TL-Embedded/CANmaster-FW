from enum import Enum
import serial
import can
import typing

CAN_EXT_BIT = 1 << 5


class CANMasterError(Enum):
    UNKNOWN = 0x00
    BUS_OVERCURRENT = 0x01
    BUS_OVERVOLTAGE = 0x02
    BUS_TRANSMIT_FAILURE = 0x03
    TRANSMIT_BUFFER_FULL = 0x04


def _u32_to_bytes(word: int) -> bytearray:
    return [
         word & 0xFF,
        (word >> 8) & 0xFF,
        (word >> 16) & 0xFF,
        (word >> 24) & 0xFF
    ]

def _u16_to_bytes(word: int) -> bytearray:
    return [
        word & 0xFF,
        (word >> 8) & 0xFF
    ]

def _u32_from_bytes(bytes: bytearray) -> int:
    return (bytes[0]) | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)

def _u16_from_bytes(bytes: bytearray) -> int:
    return (bytes[0]) | (bytes[1] << 8)


class CANMaster:
    def __init__(self, port: str ):
        self.port = serial.Serial(port, timeout=0.1)
        self.buffer = bytearray()
        self.error_callback = None

    def send(self, msg: can.Message):

        header = 0xC0
        if msg.is_extended_id:
            header |= CAN_EXT_BIT
        header |= len(msg.data)
        
        data = bytearray()
        data.append(0xAA)
        data.append(header)
        if msg.is_extended_id:
            data.extend(_u32_to_bytes(msg.arbitration_id))
        else:
            data.extend(_u16_to_bytes(msg.arbitration_id))
        data.extend(msg.data)
        data.append(0x55)
        self.port.write(data)
    
    def recv(self, timeout: float = None):

        # Check our current buffer for data
        msg = self._get_next_message()
        if msg is not None:
            return msg
        elif timeout is None or timeout > 0:
            # See if we have any data waiting
            self._await_data(timeout)
            return self._get_next_message()
        return None

    def on_error(self, callback: typing.Callable[[CANMasterError], None] ):
        # register a callback for the error condition
        self.error_callback = callback

    def _handle_error(self, code: int):
        if self.error_callback is not None:
            self.error_callback(CANMasterError(code))

    def _await_data(self, timeout: float = None) -> bytearray:
        # Wait for one or more bytes to be available
        self.port.timeout = timeout
        data = bytearray()
        data.extend(self.port.read(1))

        if len(data):
            # Read any other data that has shown up.
            data.extend(self.port.read(self.port.inWaiting()))

        self.buffer.extend(data)

    def _get_next_message(self) -> can.Message | None:
        while len(self.buffer):
            index, msg = self._read_message(self.buffer)
            if index == 0:
                break
            self.buffer = self.buffer[index:]
            if msg is not None:
                return msg
        return None

    def _find_header(self, buffer: bytearray) -> int:
        for i in range(len(buffer)):
            if buffer[i] == 0xAA:
                return i
        return 0

    def _read_message(self, buffer: bytearray) -> tuple[int, can.Message | None]:
        # check for a start byte
        if buffer[0] != 0xAA:
            return self._find_header(buffer), None

        # is there enough data for a complete message?
        if len(buffer) < 4:
            # come back later
            return 0, None

        header = buffer[1]

        # select the decoder based on the header.
        if (header & 0xC0) == 0xC0:
            # Can message?
            return self._read_can_message(buffer, header)

        elif header == 0x15:
            # Error message?
            n, error_code = self._read_error_message(buffer)
            if error_code is not None:
                self._handle_error(error_code)
            return n, None

        else:
            # Unknown. Discard it.
            return 2, None

    def _read_can_message(self, buffer: bytearray, header: int) -> tuple[int, can.Message | None]:
        dlc = header & 0x0F
        is_extended = (header & CAN_EXT_BIT) != 0

        total_length = dlc + 3 + (4 if is_extended else 2)

        # check for remaining length
        if len(buffer) < total_length:
            return 0, None

        # check for the stop char
        if buffer[total_length-1] != 0x55:
            return total_length, None

        # we can decode the message out of the buffer
        if is_extended:
            arbitration_id = _u32_from_bytes(buffer[2:6])
            data = buffer[6:6+dlc]
        else:
            arbitration_id = _u16_from_bytes(buffer[2:4])
            data = buffer[4:4+dlc]

        return total_length, can.Message(arbitration_id=arbitration_id, data=data, is_extended_id=is_extended, dlc=dlc)

    def _read_error_message(self, buffer: bytearray) -> tuple[int, str | None]:

        # check for a complete message.
        if len(buffer) < 4:
            return 0, None

        # read the error code
        error_code = buffer[2]

        # check for the stop char
        if buffer[3] != 0x55:
            return 4, None

        return 4, error_code

    def configure(self, bitrate: int = 250000, terminator: bool = False, silent: bool = False, error_code: bool = False, filter_id: int = 0, filter_mask: int = 0) -> "CANMaster":

        flags = 0x00
        if terminator:
            flags |= 0x01
        if silent:
            flags |= 0x02
        if error_code:
            flags |= 0x04
        
        data = bytearray()
        data.append(0xAA)
        data.append(0x13)
        data.append(flags)
        data.extend(_u32_to_bytes(bitrate))
        data.extend(_u32_to_bytes(filter_id))
        data.extend(_u32_to_bytes(filter_mask))
        data.append(0x55)
        self.port.write(data)

        return self




