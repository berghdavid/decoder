"""
Main program for backend
"""

import sys
import getopt
import socket

class Server:
    """ Holds relevant server data """
    def __init__(self, port, pending, buf_size, reuse, forward, api_key):
        self.port = port
        self.pending = pending
        self.buf_size = buf_size
        self.reuse = reuse
        self.forward = forward
        self.api_key = api_key
        self.host = ""
        self.s_socket = None
        self.c_socket = None

    def init_socket(self):
        """ Start up socket connection """
        self.host = socket.gethostbyname(socket.gethostname())
        self.s_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s_socket.bind((self.host, self.port))
        self.s_socket.listen(self.pending)

    def print_self(self):
        """ Print relevant information """
        print(" ------------- Starting server -------------")
        print(f"\tHOST:    {self.host}")
        print(f"\tPORT:    {self.port}")
        print(f"\tPENDING: {self.pending}")
        print(f"\tMAX_BUF: {self.buf_size}")
        print(f"\tREUSE:   {self.reuse}")
        print(f"\tFORWARD: {self.forward}")
        print(f"\tAPI_KEY: {self.api_key}")
        print(" -------------------------------------------")

    def run(self):
        """ Start server """
        print("Running fifo parser")
        self.print_self()

        try:
            while True:
                self.c_socket, c_address = self.s_socket.accept()
                print(f"Accepted connection from {c_address}")
                data = self.c_socket.recv(2048).decode('utf-8')
                if not data:
                    break
                print(f"Received data: {data}")
                self.c_socket.send("Hello\n".encode())
                self.c_socket.close()
        except KeyboardInterrupt:
            print("Server terminated by user")

def main(argv):
    """ Main function """
    port: int = 5124
    pending: int = 512
    buf_size: int = 2048
    reuse: int = 1
    forward: str = None
    api_key: int = 0
    opts, _ = getopt.getopt(argv, "P:p:b:r:f:k:",
        ["port=", "pending=", "buf_size=", "reuse=", "forward=", "api_key="])
    try:
        for opt, arg in opts:
            if opt in ("-P", "--port"):
                port = int(arg)
            elif opt in ("-p", "--pending"):
                port = int(arg)
            elif opt in ("-b", "--buf_size"):
                port = int(arg)
            elif opt in ("-r", "--reuse"):
                port = int(arg)
            elif opt in ("-f", "--forward"):
                port = int(arg)
            elif opt in ("-k", "--api_key"):
                port = int(arg)
    except ValueError:
        print(f"Could not parse arguments from '{argv}'")
        print("Pass arguments in format: P:p:b:r:f:k:")
        return
    server = Server(port, pending, buf_size, reuse, forward, api_key)
    server.init_socket()
    server.run()

if __name__ == "__main__":
    main(sys.argv[1:])
