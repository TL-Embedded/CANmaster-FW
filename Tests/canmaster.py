import serial
import can

CAN_EXT_BIT = 1 << 5

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

    def _read_data(self) -> bytearray:
        ready = self.port.inWaiting()
        if ready > 0:
            return self.port.read(ready)
        return bytearray()

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
        if len(buffer) < 5:
            # come back later
            return 0, None

        header = buffer[1]
        # check for a header
        if header & 0xC0 != 0xC0:
            return 2, None

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

    def configure(self, bitrate: int = 250000, terminator: bool = False, filter_id: int = 0, filter_mask: int = 0) -> "CANMaster":

        data = bytearray()
        data.append(0xAA)
        data.append(0x13)
        data.append(0x01 if terminator else 0x00)
        data.extend(_u32_to_bytes(bitrate))
        data.extend(_u32_to_bytes(filter_id))
        data.extend(_u32_to_bytes(filter_mask))
        data.append(0x55)
        self.port.write(data)

        return self




