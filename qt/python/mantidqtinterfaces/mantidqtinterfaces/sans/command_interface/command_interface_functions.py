# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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
