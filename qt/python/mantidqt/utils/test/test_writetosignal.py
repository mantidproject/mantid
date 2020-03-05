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
import unittest

from mantid.py3compat.mock import patch
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.writetosignal import WriteToSignal


class Receiver(QObject):
    captured_txt = None

    def capture_text(self, txt):
        self.captured_txt = txt


@start_qapplication
class WriteToSignalTest(unittest.TestCase):

    def test_run_with_output_present(self):
        with patch("sys.stdout") as mock_stdout:
            mock_stdout.fileno.return_value = 10
            writer = WriteToSignal(mock_stdout)
            mock_stdout.fileno.assert_called_once_with()
            self.assertEqual(writer._original_out, mock_stdout)

    def test_run_without_output_present(self):
        with patch("sys.stdout") as mock_stdout:
            mock_stdout.fileno.return_value = -1
            writer = WriteToSignal(mock_stdout)
            mock_stdout.fileno.assert_called_once_with()
            self.assertEqual(writer._original_out, None)

    def test_connected_receiver_receives_text(self):
        with patch("sys.stdout") as mock_stdout:
            mock_stdout.fileno.return_value = -1
            recv = Receiver()
            writer = WriteToSignal(mock_stdout)
            writer.sig_write_received.connect(recv.capture_text)
            txt = "I expect to see this"
            writer.write(txt)
            QCoreApplication.processEvents()
            self.assertEqual(txt, recv.captured_txt)
            mock_stdout.fileno.assert_called_once_with()

    def test_with_fileno_not_defined(self):
        with patch('sys.stdout') as mock_stdout:
            del mock_stdout.fileno
            writer = WriteToSignal(mock_stdout)
            self.assertEqual(writer._original_out, None)


if __name__ == "__main__":
    unittest.main()
