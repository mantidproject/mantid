# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from qtpy.QtCore import QObject, Signal
from io import UnsupportedOperation


class WriteToSignal(QObject):
    """Provides a minimal file-like object that can be
    used to transform write requests to
    Qt-signals. Mainly used to communicate
    stdout/stderr across threads"""

    sig_write_received = Signal(str)

    def __init__(self, original_out):
        QObject.__init__(self)
        # If the file descriptor of the stream is < 0 then we are running in a no-external-console mode
        try:
            self._original_out = original_out
            if not hasattr(original_out, 'fileno') or original_out.fileno() < 0:
                self._original_out = None
        except UnsupportedOperation:
            # In some cases fileno may be defined but throw this instead,
            # such as in the case of io.StringIO
            self._original_out = None

    def closed(self):
        return False

    def flush(self):
        pass

    def isatty(self):
        return False

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
                Scripts containing unicode characters could fail to run if mantid is not started from the
                terminal on unix systems. The script runs fine if this exception is caught and discarded.
                """
                pass
        # always write to the message log
        self.sig_write_received.emit(txt)
