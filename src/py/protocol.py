"""
Abstract class for parsing received data.
"""

import re
from abc import ABC, abstractmethod
from utils import eprint

class Protocol(ABC):
    """ Abstract class for parsing received data. """

    # Contains all available parameters for a given protocol.
    ALL_PARAMS: [str]

    def __init__(self):
        """ Initialize class local variables. """
        self.recv: str = ""
        self.send: str = ""
        self.forw: str = ""
        self.params: dict = {}
        for element in self.ALL_PARAMS:
            self.params[element] = ""

    def setup_forwarding(self, forw: str) -> bool:
        """
        Setup forwarding url

        :return: True if setup was successful, otherwise False.
        """
        if not forw:
            self.forw = ""
            return True
        reg = re.compile(r'\{\{([^}]+)\}\}')
        hits = reg.findall(forw)
        for hit in hits:
            if hit not in self.ALL_PARAMS:
                eprint(f"Could not find parameter {hit} in parameter list of "
                       f"{self.__class__.__name__}. Disabling data forwarding.")
                self.forw = ""
                return False
        self.forw = forw
        return True

    def forward_data(self):
        """ Forward all data to the forwarding url """
        # TODO: Fix forwarding, probably using CURL
        if self.forw == "":
            return
        print("Sending stuff...")

    @abstractmethod
    def parse_data(self, rec: str) -> bool:
        """
        Parse the received data and store in local object variables.

        :return: True if parsing was successful, otherwise False.
        """

    @abstractmethod
    def should_respond(self) -> bool:
        """ Whether or not the server should respond according to procotol """

    @abstractmethod
    def build_resp(self) -> bool:
        """ Build the response message """

    @abstractmethod
    def print(self):
        """ Print data """
        print(self.recv)
        print(self.params)
