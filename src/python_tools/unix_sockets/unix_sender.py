import socket
from struct import pack


class StoppableSender:
    def __init__(self):
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)

    def stop(self):
        self.socket.close()

    def connect(self, address):
        try:
            self.socket.bind(address)
        except socket.error as e:
            print(e)
            exit(1)

    def send_raw(self, message):
        try:
            self.socket.sendto(bytes("/tmp/test", encoding="utf-8"),message.encode())
        except Exception as e:
            print(e)
            exit(1)

    def send_encoded_proto(self, encoded_proto):
        header = pack(">I", len(encoded_proto))
        try:
            self.socket.sendall(header)
            self.socket.sendall(encoded_proto)
        except Exception as e:
            print(e)
            exit(1)


def main():
    address = "/tmp/dynamic_param"
    sender = StoppableSender()
    sender.connect(address)
    sender.send_raw("nice")
    # TODO send stuff


if __name__ == "__main__":
    main()
