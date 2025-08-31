import canmaster
import can
import time


def list_canmasters() -> list[str]:
    from serial.tools.list_ports import comports
    ports = []
    for port in comports():
        if port.vid == 0x0483 and port.pid == 0x5740:
            ports.append(port.device)
    return ports


if __name__ == "__main__":
    
    devices = list_canmasters()

    busa = canmaster.CANMaster(devices[1])
    busb = canmaster.CANMaster(devices[0])
    busa.configure(bitrate=50000, terminator=True, silent=False, error_code=True)
    busb.configure(bitrate=50000, terminator=False, silent=False, error_code=True)
    busa.on_error(lambda err: print(f"Bus A Error: {err}"))
    busb.on_error(lambda err: print(f"Bus B Error: {err}"))

    
    for i in range(100):

        busa.send(can.Message(arbitration_id=0x123, data=[0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88], is_extended_id=True))
        busb.recv(0.1)
        busb.send(can.Message(arbitration_id=0x124, data=[0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88], is_extended_id=True))
        busa.recv(0.1)
        time.sleep(0.25)





