"""
General utility functions
"""

import sys

def eprint(*args, **kwargs):
    """ Print error message to stderr """
    print("Error - ", end="", file=sys.stderr)
    print(*args, file=sys.stderr, **kwargs)
