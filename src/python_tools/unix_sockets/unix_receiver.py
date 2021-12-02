import socketserver
from struct import unpack
import os


class StoppableServer:
    stop = False

    def __init__(self, server):
        self.server = server

    def serve_till_stopped(self):
        while not self.stop:
            self.server.handle_request()

    def force_stop(self):
        self.stop = True
        self.server.server_close()

    def start(self):
        self.stop = False
        self.serve_till_stopped()


class Session(socketserver.BaseRequestHandler):
    def __init__(self, proto_type, handle_callback, *args, **keys):
        self.handle_callback = handle_callback
        self.proto_type = proto_type
        super().__init__(*args, **keys)

    def handle(self):
        # header = self.request.recv(4)
        # (message_length,) = unpack(">I", header)  # unpack always returns a tuple.

        # message = self.request.recv(message_length)
        message = self.request[0]
        print(self.request)


def handler_factory(proto_type, handle_callback):
    def create_handler(*args, **keys):
        return Session(proto_type, handle_callback, *args, **keys)

    return create_handler


def handle_proto(proto_type):
    print("CALELD")
    print(proto_type)


def main():
    address = "/tmp/sendVision"

    try:
        os.unlink(address)
    except OSError as e:
        print(e)
        pass

    receiver = StoppableServer(
        socketserver.UnixDatagramServer(
            address, handler_factory(None, handle_proto)
        )
    )
    receiver.start()


if __name__ == "__main__":
    main()
