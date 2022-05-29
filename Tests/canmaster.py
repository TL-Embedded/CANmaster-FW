from email.message import Message
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
        self.port.timeout = timeout
        self.buffer.extend(self.port.read())
        while True:
            index, msg = self._read_message(self.buffer)
            if index == 0:
                break
            self.buffer = self.buffer[index:]
            if msg is not None:
                return msg

    def _read_message(self, buffer: bytearray) -> tuple[int, can.Message | None]:
        # check for a start byte
        if buffer[0] != 0xAA:
            return 1, None

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
             
        

    def configure(self, bitrate: int = 250000, terminator: bool = False, filter_id: int = 0x00000000, filter_mask: int = 0xFFFFFFFF):

        data = bytearray()
        data.append(0xAA)
        data.append(0x55)
        data.append(0x13)
        data.extend(_word_to_bytes(bitrate))
        data.extend(_word_to_bytes(filter_id))
        data.extend(_word_to_bytes(filter_mask))
        data.append(0x55)
        self.port.write(data)




