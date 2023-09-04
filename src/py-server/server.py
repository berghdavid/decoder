"""
Main program for backend
"""

import sys
import getopt
from bottle import run
import api # pylint: disable=unused-import

def main(argv):
    """ Main function """
    host: str = "localhost"
    port: int = 5124
    opts, _ = getopt.getopt(argv, "h:p:", ["host=", "port="])
    for opt, arg in opts:
        if opt in ("-h", "--host"):
            host = str(arg)
        elif opt in ("-p", "--port"):
            port = int(arg)
    print(f"Running server at {str(host)}:{port}")
    run(host=host, port=port, quiet=True)


if __name__ == "__main__":
    main(sys.argv[1:])
