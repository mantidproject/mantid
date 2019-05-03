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

from qtpy.QtCore import QCoreApplication, QObject
import sys
import unittest

from mantid.py3compat.mock import patch
from mantidqt.utils.qt.testing import GuiTest
from mantidqt.utils.writetosignal import WriteToSignal


class Receiver(QObject):
    captured_txt = None

    def capture_text(self, txt):
        self.captured_txt = txt


class WriteToSignalTest(GuiTest):

    def setUp(self):
        if not hasattr(sys.stdout, "fileno"):
            # if not present in the test stdout, then add it as an
            # attribute so that mock can replace it later.
            sys.stdout.fileno = None

    def test_run_with_output_present(self):
        with patch("sys.stdout.fileno", return_value=10) as mock_fileno:
            writer = WriteToSignal(sys.stdout)
            mock_fileno.assert_called_once_with()
            self.assertEqual(writer._original_out, sys.stdout)

    def test_run_without_output_present(self):
        with patch("sys.stdout.fileno", return_value=-1) as mock_fileno:
            writer = WriteToSignal(sys.stdout)
            mock_fileno.assert_called_once_with()
            self.assertEqual(writer._original_out, None)

    def test_connected_receiver_receives_text(self):
        with patch("sys.stdout.fileno", return_value=1) as mock_fileno:
            recv = Receiver()
            writer = WriteToSignal(sys.stdout)
            writer.sig_write_received.connect(recv.capture_text)
            txt = "I expect to see this"
            writer.write(txt)
            QCoreApplication.processEvents()
            self.assertEqual(txt, recv.captured_txt)
            mock_fileno.assert_called_once_with()

    def test_with_fileno_not_defined(self):
        with patch('sys.stdout') as mock_stdout:
            del mock_stdout.fileno
            writer = WriteToSignal(sys.stdout)
            self.assertEqual(writer._original_out, None)


if __name__ == "__main__":
    unittest.main()
