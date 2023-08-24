""" Server """
import socket

# Uplink:   $$<pack-len>,<ID>,<work-no>,<cmd-code>,<cmd-para>*<checksum>\r\n
# Downlink: ##<pack-len>,<ID>,<work-no>,<cmd-code>,<cmd-para>*<checksum>\r\n

class Package:
    """ Contains Fifo package information """

    def __init__(self, data):
        if data[:2] != '$$':
            return
        data = data[2:]
        data = data[:-2]
        splitted = data.split(',')
        if len(splitted) != 5:
            return
        self.pack_len = splitted[0]
        self.id = splitted[1]
        self.work_no = splitted[2]
        self.cmd_code = splitted[3]
        last = splitted[4].split('*')
        if len(last) != 2:
            return
        self.cmd_para = last[0]
        self.checksum = last[1]

    def print(self):
        """ Print attributes """
        print(f'$${self.pack_len},{self.id},{self.work_no},{self.cmd_code},' \
              '{self.cmd_para}*{self.checksum}\r\n')


def server_program():
    """ Run server """
    # get the hostname
    host = socket.gethostname()
    port = 5142  # initiate port no above 1024

    server_socket = socket.socket()  # get instance
    # look closely. The bind() function takes tuple as argument
    server_socket.bind((host, port))  # bind host address and port together

    # configure how many client the server can listen simultaneously
    server_socket.listen(2)
    conn, address = server_socket.accept()  # accept new connection
    print("Connection from: " + str(address))
    while True:
        data = conn.recv(2048).decode() # 2048 Bytes
        if not data:
            break
        print("from connected user: " + str(data))

        fifo = Package(data)
        fifo.print()

        data = input(' -> ')
        conn.send(data.encode())

    conn.close()


if __name__ == '__main__':
    server_program()
