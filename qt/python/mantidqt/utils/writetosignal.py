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

    sig_write_received = Signal(str)

    def __init__(self, original_out):
        QObject.__init__(self)
        # Check if the output stream is a printable terminal.
        if not original_out.isatty():
            self._original_out = None
        else:
            self._original_out = original_out

    def closed(self):
        return False

    def flush(self):
        pass

    def write(self, txt):
        if self._original_out:
            try:
                self._original_out.write(txt)
            except IOError as e:
                self.sig_write_received.emit("Error: Unable to write to the console of the process.\n"
                                             "This error is not related to your script's execution.\n"
                                             "Original error: {}\n\n".format(str(e)))
            except UnicodeEncodeError:
                """
                Scripts containing unicode characters could fail to run if the original_out does not
                support those characters. If this occurs, just don't print to that terminal and continue.
                """
                pass
        # always write to the message log
        self.sig_write_received.emit(txt)
