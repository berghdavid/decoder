"""
Fifo protocol parser
"""
from datetime import datetime
from protocol import Protocol
from utils import eprint

class Fifo(Protocol):
    """ Parses data according to Fifo protocol """

    ALL_PARAMS = [
        "pack-len", "id", "work-no", "cmd-code", "alm-code", "date-time",
        "MCC|MNC|LAC|CI", "bat-v", "bat-level", "status", "loc-type",
        "fix-flag", "speed", "salt-num", "lat", "lon", "wifi-ap", "bat-ad",
        "checksum"
    ]
    # Package headers
    HEADERS = [
        "pack-len", "id", "work-no", "cmd-code", "params", "checksum"
    ]
    # Parameters for command code A03 when loc-type == 0
    A03_0 = [
        "alm-code", "date-time", "MCC|MNC|LAC|CI", "bat-v", "bat-level", "status",
        "loc-type", "fix-flag", "speed", "salt-num", "lat", "lon"
    ]
    # Parameters for command code A03 when loc-type == 1
    A03_1 = [
        "alm-code", "date-time", "MCC|MNC|LAC|CI", "bat-v", "bat-level", "status",
        "loc-type", "wifi-ap"
    ]
    # Parameters for command code A10
    A10 = [
        "status", "bat-ad"
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
        param_args = ""
        while rec != "":
            rec = rec.split(',', 1)
            header = rec[0]
            rec = rec[1]
            self.params[Fifo.HEADERS[i]] = header
            if i == 3:
                last = rec.rsplit("*")
                if len(last) != 2:
                    eprint("Found no * sign before checksum")
                    return False
                param_args = last[0]
                self.params["checksum"] = last[1]
                break
            i += 1
        return self.parse_params(param_args)

    def parse_params(self, param_args) -> bool:
        """ Parse parameters from header into params dict """
        code = self.params["cmd-code"]
        if code == "A03":
            return self.parse_a03_params(param_args)
        if code == "A10":
            return self.parse_a10_params(param_args)
        eprint(f"Invalid <cmd-code> {code}")
        return False

    def parse_a03_params(self, param_args) -> bool:
        """ Parse params according to A03 package """
        paramss = param_args.split(",")
        if len(paramss) < 8:
            eprint(f"Too few parameters in <cmd-para> for A03: {paramss}")
            return False
        for i, par in enumerate(paramss[:7]):
            self.params[Fifo.A03_0[i]] = par

        # Protocol now varies depending on if loc-type==0 or loc-type==1
        if self.params["loc-type"] == "0":
            if len(paramss) != 12:
                eprint("Number of parameters was not 12 when loc-type==0")
                return False
            for i, par in enumerate(paramss[7:]):
                self.params[Fifo.A03_0[7 + i]] = par
        else:
            if len(paramss) != 8:
                eprint("Number of parameters was not 8 when loc-type==1")
                return False
            self.params[Fifo.A03_1[7]] = paramss[7]
        return True

    def parse_a10_params(self, param_args) -> bool:
        """ Parse params according to A10 package """
        paramss = param_args.split(",")
        if len(paramss) >= 8:
            eprint("Found more than 8 parameters for A10 package")
            return False
        for i, par in enumerate(paramss):
            self.params[Fifo.A10[i]] = par
        return True

    def should_respond(self) -> bool:
        return self.params["cmd-code"] == "A03"

    def build_resp(self) -> bool:
        now = datetime.now()
        dt_str = now.strftime("%y%m%d%H%M%S")
        pack = f",{self.params['id']},{self.params['work-no']},A03,{dt_str}"
        pack_len = f"{str(len(pack))}{pack}"

        # Calculate xor checksum
        xor = 0
        for char in pack_len:
            xor = xor ^ ord(char)
        # Make decimal into hex and then turn hex into 2 char format
        # Hex formatting examples: 0x3c -> 3c, 0xf -> 0f
        xor_str = str(hex(xor))[2:].rjust(2, '0')
        self.send = f"##{pack_len}*{xor_str}\r\n"
        return True

    def print(self):
        print(self.recv)
        print(self.params)
