"""
Main program for backend
"""

import sys
import getopt
import socket
from protocol import Protocol
from fifo import Fifo
from utils import eprint

class Server:
    """ Holds relevant server data """
    def __init__(self, prot: Protocol, params):
        self.protocol: Protocol = prot()
        self.params = params
        self.host = ""
        self.s_socket = None
        self.c_socket = None

    def init_socket(self):
        """ Start up socket connection """
        self.host = socket.gethostbyname(socket.gethostname())
        self.s_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s_socket.bind((self.host, self.params["port"]))
        self.s_socket.listen(self.params["pending"])

    def print_self(self):
        """ Print relevant information """
        print(" ------------- Starting server -------------")
        print(f"\thost:     {self.host}")
        print(f"\tport:     {self.params['port']}")
        print(f"\tpending:  {self.params['pending']}")
        print(f"\tbuf_size: {self.params['buf_size']}")
        print(f"\treuse:    {self.params['reuse']}")
        print(f"\tforward:  {self.protocol.forw}")
        print(" -------------------------------------------")

    def run(self):
        """ Start server """
        protocol: Protocol = self.protocol
        protocol.setup_forwarding(self.params["forward"])
        self.print_self()
        try:
            while True:
                self.c_socket, _ = self.s_socket.accept()
                data = self.c_socket.recv(2048).decode('utf-8')
                protocol.log_event("< Fifo", data)
                if protocol.parse_data(data):
                    if protocol.should_respond():
                        protocol.build_resp()
                        self.c_socket.send(protocol.send.encode())
                        protocol.log_event("> Fifo", protocol.send)
                    protocol.forward_data()
                self.c_socket.close()
        except KeyboardInterrupt:
            print("Server terminated by user")

def main(argv):
    """ Main function """
    params = {
        "port":     5124,
        "pending":  512,
        "buf_size": 2048,
        "reuse":    1,
        "forward":  ""
    }
    opts, _ = getopt.getopt(argv, "P:p:b:r:f:k:",
        ["port=", "pending=", "buf_size=", "reuse=", "forward=", "api_key="])
    try:
        for opt, arg in opts:
            if opt in ("-P", "--port"):
                params["port"] = int(arg)
            elif opt in ("-p", "--pending"):
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
    server = Server(Fifo, params)
    server.init_socket()
    server.run()

if __name__ == "__main__":
    main(sys.argv[1:])
