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

from qtpy.QtCore import QObject, Signal


class WriteToSignal(QObject):
    """Provides a minimal file-like object that can be
    used to transform write requests to
    Qt-signals. Mainly used to communicate
    stdout/stderr across threads"""

    def __init__(self, original_out):
        QObject.__init__(self)
        # If the file descriptor of the stream is < 0 then we are running in a no-external-console mode
        if original_out.fileno() < 0:
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
            try:
                self.__original_out.write(txt)
            except IOError as e:
                self.sig_write_received.emit("Error: Unable to write to the console of the process.\n"
                                             "This error is not related to your script's execution.\n"
                                             "Original error: {}\n\n".format(str(e)))

        # always write to the message log
        self.sig_write_received.emit(txt)
