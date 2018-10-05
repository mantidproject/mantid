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

# std imports

# 3rdparty imports
from qtpy.QtCore import QObject, Signal


class WriteToSignal(QObject):
    """Provides a minimal file-like object that can be
    used to transform write requests to
    Qt-signals. Mainly used to communicate
    stdout/stderr across threads"""
    def __init__(self, original_out):
        QObject.__init__(self)
        self.__original_out = original_out

    sig_write_received = Signal(str)

    def closed(self):
        return False

    def flush(self):
        pass

    def isatty(self):
        return False

    def write(self, txt):
        # write to the console
        self.__original_out.write(txt)
        # emit the signal which will write to logging
        self.sig_write_received.emit(txt)
