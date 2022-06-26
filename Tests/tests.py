from psutil import boot_time
import canmaster
import can
import time
import threading

TEST_ID = 0x00020130
TEST_EXT = True

def test_message(counter: int) -> can.Message:
    data = [
        0x11,
        0x22,
        0x33,
        0x44,
        (counter >> 24) & 0xFF,
        (counter >> 16) & 0xFF,
        (counter >> 8) & 0xFF,
        counter & 0xFF
    ]
    return can.Message(
        arbitration_id=TEST_ID,
        data=data,
        is_extended_id=TEST_EXT,
        dlc=len(data)
        )

def get_counter(msg: can.Message) -> int:
    if msg.arbitration_id != TEST_ID:
        return 0
    return (msg.data[4] << 24) | (msg.data[5] << 16) | (msg.data[6] << 8) | msg.data[7]


class TxBusThread(threading.Thread):
    def __init__(self, bus: canmaster.CANMaster, tx_rate: float):
        super().__init__()
        self.bus = bus
        self.running = True
        self.counter = 0
        self.sent = 0
        self.tx_rate = tx_rate
        self.tx_block = 64

    def run(self):
        t = time.time()
        while self.running:
            now = time.time()
            elapsed = now - t
            t = now

            to_send = int(self.tx_rate * elapsed)
            if to_send > 0:
                for i in range(to_send):
                    self.send_next()
        
            time.sleep(0.005)

    def send_next(self):
        self.bus.send(test_message(self.counter))
        self.counter += 1
        self.sent += 1

    def stop(self):
        self.running = False


class RxBusThread(threading.Thread):
    def __init__(self, bus: canmaster.CANMaster):
        super().__init__()
        self.bus = bus
        self.running = True
        self.next_counter = 0

        self.recieved = 0
        self.errors = 0

    def run(self):
        while self.running:
            self.recv_next()

    def recv_next(self):
        msg = self.bus.recv(0.1)
        if msg != None:
            self.recieved += 1
            counter = get_counter(msg)
            if counter != self.next_counter:
                print ("Error: %d -> %d" % (self.next_counter, counter))
                self.errors += 1
            self.next_counter = counter + 1

    def stop(self):
        self.running = False


class PingPongThread(threading.Thread):
    def __init__(self, busa: canmaster.CANMaster, busb: canmaster.CANMaster):
        super().__init__()
        self.busa = busa
        self.busb = busb
        self.running = True
        self.next_counter = 0

        self.recieved = 0
        self.errors = 0
        self.sent = 0

    def run(self):
        while self.running:
            self._send_recv(self.busa, self.busb, self.next_counter)
            self.next_counter += 1
            self._send_recv(self.busb, self.busa, self.next_counter)
            self.next_counter += 1

    def _send_recv(self, busa: canmaster.CANMaster, busb: canmaster.CANMaster, counter: int):
        busa.send(test_message(counter))
        self.sent += 1
        msg = busb.recv(0.1)
        if msg != None:
            counter = get_counter(msg)
            if counter != self.next_counter:
                print ("Error: %d -> %d" % (self.next_counter, counter))
                self.errors += 1
            else:
                self.recieved += 1

    def stop(self):
        self.running = False


def test_transmission(busa: canmaster.CANMaster, busb: canmaster.CANMaster, config: dict = {}) -> dict:

    # only enable terminator on one side - in case the other has failed.
    busa.configure(config['bitrate'], True)
    busb.configure(config['bitrate'], False)

    tx_thread = TxBusThread(busa, config['tx_rate'])
    rx_thread = RxBusThread(busb)

    tx_thread.start()
    rx_thread.start()

    time.sleep(config['test_time'])

    tx_thread.stop()
    time.sleep(0.1)
    rx_thread.stop()

    stats = {
        "recieved": rx_thread.recieved,
        "sent": tx_thread.sent,
        "errors": rx_thread.errors,
        "rate": rx_thread.recieved / config['test_time']
    }
    return stats

def test_pingpong_transmission(busa: canmaster.CANMaster, busb: canmaster.CANMaster, config: dict = {}) -> dict:
    
        # only enable terminator on one side - in case the other has failed.
        busa.configure(config['bitrate'], True)
        busb.configure(config['bitrate'], False)
    
        pp_thread = PingPongThread(busa, busb)
    
        pp_thread.start()
    
        time.sleep(config['test_time'])
    
        pp_thread.stop()
    
        stats = {
            "recieved": pp_thread.recieved,
            "sent": pp_thread.sent,
            "errors": pp_thread.errors,
            "rate": pp_thread.recieved / config['test_time']
        }
        return stats


def print_stats(stats: dict):
    print("Recieved: %d" % stats["recieved"])
    print("Sent: %d" % stats["sent"])
    print("Errors: %d" % stats["errors"])
    print("Rate: %f" % stats["rate"])


def list_canmasters() -> list[str]:
    from serial.tools.list_ports import comports
    ports = []
    for port in comports():
        if port.vid == 0x0483 and port.pid == 0x5740:
            ports.append(port.name)
    return ports

def check_stats(config: dict, stats: dict) -> bool:
    tolerance = 0.9

    expected_messages = config["test_time"] * config["tx_rate"]

    if stats["sent"] < expected_messages * tolerance:
        return False
    
    if stats["recieved"] < expected_messages * tolerance:
        return False

    if stats["sent"] != stats["recieved"]:
        return False

    if stats["errors"] > 0:
        return False

    if stats["rate"] < config["tx_rate"] * tolerance:
        return False

    return True


def main():
    ports = list_canmasters()
    if len(ports) != 2:
        print("Error: Expected 2 CAN masters, found %d" % len(ports))
        return

    config = {
        "bitrate": 250000,
        "tx_rate": 1750,
        "test_time": 5.0
    }

    busa = canmaster.CANMaster(ports[0])
    busb = canmaster.CANMaster(ports[1])

    print("Testing bus A -> bus B")
    atob = test_transmission(busa, busb, config)
    print_stats(atob)
    print("Testing bus B -> bus A")
    btoa = test_transmission(busb, busa, config)
    print_stats(btoa)

    if check_stats(config, atob) and check_stats(config, btoa):
        print("Test passed")
    else:
        print("Test failed")



if __name__ == "__main__":
    main()