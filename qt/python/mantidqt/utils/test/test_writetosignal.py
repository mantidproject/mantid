#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
