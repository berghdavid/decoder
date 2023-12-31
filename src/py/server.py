"""
Main program for backend
"""

import threading
import sys
import getopt
import socket
from protocol import Protocol
from fifo import Fifo
from meitrack import Meitrack
from utils import eprint

class Server:
    """ Holds relevant server data """
    def __init__(self, protocols: [Protocol], params):
        self.threads: [] = [None] * len(protocols)
        self.protocols: [Protocol] = []
        for protocol in protocols:
            self.protocols.append(protocol())
        self.params = params
        self.host = '0.0.0.0'
        self.s_sockets: [socket] = []
        for protocol in self.protocols:
            self.init_socket(protocol)
            protocol.setup_forwarding(params["forward"])

    def init_socket(self, protocol: Protocol):
        """ Start up socket connection """
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((self.host, protocol.PORT))
        sock.listen(self.params["pending"])
        self.s_sockets.append(sock)

    def print_self(self):
        """ Print relevant information """
        print(" ------------- Starting server -------------")
        print(f"\thost:     {self.host}")
        print(f"\tpending:  {self.params['pending']}")
        print(f"\tbuf_size: {self.params['buf_size']}")
        print(f"\treuse:    {self.params['reuse']}")
        print(f"\tforward:  {self.protocols[0].forw}")
        print(" -------------------------------------------")

    def run(self, worker: int):
        """ Run a protocol parser given by index worker """
        protocol: Protocol = self.protocols[worker]
        s_socket = self.s_sockets[worker]
        c_socket = None
        print(f"Running {protocol.__class__.__name__} parser on port "
              f"{protocol.PORT}")
        if protocol.forw != "":
            print("{protocol.__class__.__name__} forwarding to {protocol.forw}")
        try:
            while True:
                c_socket, _ = s_socket.accept()
                data = c_socket.recv(2048).decode('utf-8')
                protocol.log_event(f"< {protocol.__class__.__name__}", data)
                if protocol.parse_data(data):
                    if protocol.should_respond():
                        protocol.build_resp()
                        c_socket.send(protocol.send.encode())
                        protocol.log_event(f"> {protocol.__class__.__name__}", protocol.send)
                    protocol.forward_data()
                c_socket.close()
        except KeyboardInterrupt:
            print("Server terminated by user")

    def run_all(self):
        """ Start server """
        self.print_self()
        for i in range(len(self.protocols)):
            self.threads[i] = threading.Thread(target=self.run, args=(i,))
            self.threads[i].start()

def main(argv):
    """ Main function """
    params = {
        "pending":  512,
        "buf_size": 2048,
        "reuse":    1,
        "forward":  ""
    }
    opts, _ = getopt.getopt(argv, "P:p:b:r:f:k:",
        ["pending=", "buf_size=", "reuse=", "forward=", "api_key="])
    try:
        for opt, arg in opts:
            if opt in ("-p", "--pending"):
                params["pending"] = int(arg)
            elif opt in ("-b", "--buf_size"):
                params["buf_size"] = int(arg)
            elif opt in ("-r", "--reuse"):
                params["reuse"] = int(arg)
            elif opt in ("-f", "--forward"):
                params["forward"] = arg
    except ValueError:
        eprint(f"Could not parse arguments from '{argv}'")
        eprint("Pass arguments in format: P:p:b:r:f:k:")
        return
    server = Server([Fifo, Meitrack], params)
    server.run_all()

if __name__ == "__main__":
    main(sys.argv[1:])
