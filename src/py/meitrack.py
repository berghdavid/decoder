"""
Meitrack protocol parser
"""

from protocol import Protocol
from utils import eprint

class Meitrack(Protocol):
    """ Parses data according to Meitrack protocol """

    PORT = 5020
    ALL_PARAMS = [
        "work-no", "pack-len", "id", "cmd-code", "cache-records", "packets",
        # TODO: Add all params
        "checksum"
    ]
    # Package headers
    HEADERS = [
        "pack-len", "id", "work-no", "cmd-code", "packets", "checksum"
    ]

    def parse_data(self, rec: str) -> bool:
        self.recv = rec
        if not rec or len(rec) < 10:
            eprint("Package '{rec}' is too small")
            return False
        if rec[:2] != "$$" or rec[-2:] != "\r\n":
            eprint("Contained no initial $$ and no ending \\r\\n")
            return False
        rec = rec[2:-2]
        i = 0
        packets = ""
        while rec != "":
            rec = rec.split(',', 1)
            header = rec[0]
            rec = rec[1]
            self.params[Meitrack.HEADERS[i]] = header
            if i == 3:
                last = rec.rsplit("*")
                if len(last) != 2:
                    eprint("Found no * sign before checksum")
                    return False
                packets = last[0]
                self.params["checksum"] = last[1]
                break
            i += 1
        # TODO: Parse data packets
        return self.parse_params(packets)

    def parse_params(self, param_args) -> bool:
        """ Parse parameters from header into params dict """
        self.params["packets"] = param_args
        return True

    def should_respond(self) -> bool:
        return self.params["cmd-code"] == "AAC" or \
                self.params["cmd-code"] == "AAE"

    def build_resp(self) -> bool:
        # TODO: Build content for response
        content = "no_content"
        pack = f",{self.params['id']},{self.params['cmd-code']},{content}"
        pack = f"@@{self.params['work-no']},{str(len(pack) + 2)},{pack}*"

        # Calculate xor checksum
        xor = 0
        for char in pack:
            xor = xor ^ ord(char)
        # Make decimal into hex and then turn hex into 2 char format
        # Hex formatting examples: 0x3c -> 3c, 0xf -> 0f
        xor_str = str(hex(xor))[2:].rjust(2, '0')
        self.send = f"{pack}*{xor_str}\r\n"
        return True
