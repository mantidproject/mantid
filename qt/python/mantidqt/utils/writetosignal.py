# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import)

# 3rdparty imports
import sys
from qtpy.QtCore import QObject, Signal


# std imports


class WriteToSignal(QObject):
    """Provides a minimal file-like object that can be
    used to transform write requests to
    Qt-signals. Mainly used to communicate
    stdout/stderr across threads"""

    def __init__(self, original_out):
        QObject.__init__(self)
        # If the file descriptor of the stream is -2 then we are running in a no-external-console mode
        if self.__original_out.fileno() == -2:
            self.__original_out = None
        else:
            self.__original_out = original_out

    sig_write_received = Signal(str)

    def closed(self):
        return False

    def flush(self):
        pass

    def isatty(self):
        return False

    def write(self, txt):
        if self.__original_out:
            # write to the console
            try:
                self.__original_out.write(txt)
            except IOError:
                self.sig_write_received.emit("Error: Unable to write to the console of the process.\n"
                                             "This error is not related to your script's execution.\n\n")

        # always write to the message log
        self.sig_write_received.emit(txt)
