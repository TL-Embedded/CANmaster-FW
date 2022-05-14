from can.interfaces.seeedstudio import SeeedBus
import can
import time


def test_message(counter):
    data = [
        0x00,
        0x00,
        0x00,
        0x00,
        (counter >> 24) & 0xFF,
        (counter >> 16) & 0xFF,
        (counter >> 8) & 0xFF,
        counter & 0xFF
    ]
    return can.Message(
        arbitration_id=0x00102030,
        data=data,
        is_extended_id=True,
        dlc=len(data)
        )

def compare_message(msg1, msg2):
    if msg1 == None or msg2 == None:
        return False
    return msg1.arbitration_id == msg2.arbitration_id and msg1.data == msg2.data

def main():

    bitrate = 250000

    busa = SeeedBus("COM7", bitrate=bitrate)
    busb = SeeedBus("COM8", bitrate=bitrate)

    # Test that the buses are connected
    for i in range(0, 10):

        tx = test_message(i)

        busa.send(tx)
        rx = busb.recv(timeout=0.1)
        assert compare_message(tx, rx)
    
    print("Testing complete")


if __name__ == "__main__":
    main()