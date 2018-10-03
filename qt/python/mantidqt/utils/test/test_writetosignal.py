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
import unittest

# 3rdparty
from qtpy.QtCore import QCoreApplication, QObject

# local imports
from mantidqt.utils.qt.test import GuiTest
from mantidqt.utils.writetosignal import WriteToSignal


class Receiver(QObject):
    captured_txt = None

    def capture_text(self, txt):
        self.captured_txt = txt


class WriteToSignalTest(GuiTest):

    def test_connected_receiver_receives_text(self):
        recv = Receiver()
        writer = WriteToSignal()
        writer.sig_write_received.connect(recv.capture_text)
        txt = "I expect to see this"
        writer.write(txt)
        QCoreApplication.processEvents()
        self.assertEqual(txt, recv.captured_txt)


if __name__ == "__main__":
    unittest.main()
