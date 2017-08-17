from __future__ import (absolute_import, division, print_function)
from mantid.kernel import Logger


VERBOSE = False
sans_log = Logger("SANS")


# Print a message and log it if the
def print_message(message, log=True, no_console=False):
    if log and VERBOSE:
        sans_log.notice(message)
    if not no_console:
        print(message)


def warning_message(message):
    sans_log.warning(message)
