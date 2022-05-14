from can.interfaces.seeedstudio import SeeedBus
import can
import time
import threading

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

def get_counter(msg):
    return (msg.data[4] << 24) | (msg.data[5] << 16) | (msg.data[6] << 8) | msg.data[7]

def compare_message(msg1, msg2):
    if msg1 == None or msg2 == None:
        return False
    return msg1.arbitration_id == msg2.arbitration_id and msg1.data == msg2.data


class TxBusThread(threading.Thread):
    def __init__(self, bus, tx_rate):
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
    def __init__(self, bus):
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
        msg = self.bus.recv()
        if msg != None:
            self.recieved += 1
            counter = get_counter(msg)
            if counter != self.next_counter:
                print ("Error: %d -> %d" % (self.next_counter, counter))
                self.errors += 1
            self.next_counter = counter + 1

    def stop(self):
        self.running = False


def main():

    bitrate = 250000
    test_time = 5.0
    tx_rate = 1650

    busa = SeeedBus("COM7", bitrate=bitrate)
    busb = SeeedBus("COM8", bitrate=bitrate)

    tx_thread = TxBusThread(busa, tx_rate)
    rx_thread = RxBusThread(busb)

    tx_thread.start()
    rx_thread.start()

    time.sleep(test_time)

    tx_thread.stop()
    time.sleep(0.1)
    rx_thread.stop()

    print("Recieved: %d" % rx_thread.recieved)
    print("Errors: %d" % rx_thread.errors)
    print("Sent: %d" % tx_thread.sent)
    print("Message rate: %d" % (rx_thread.recieved / test_time))



if __name__ == "__main__":
    main()