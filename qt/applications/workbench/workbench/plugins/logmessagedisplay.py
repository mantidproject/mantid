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
from __future__ import (absolute_import, unicode_literals)

# std imports
import sys

# 3rdparty imports
from mantidqt.utils.writetosignal import WriteToSignal
from mantidqt.widgets.messagedisplay import MessageDisplay
from qtpy.QtWidgets import QHBoxLayout

# local imports
from workbench.plugins.base import PluginWidget

# Default logs at notice
DEFAULT_LOG_PRIORITY = 5


class LogMessageDisplay(PluginWidget):

    def __init__(self, parent):
        super(LogMessageDisplay, self).__init__(parent)

        # layout
        self.display = MessageDisplay(parent)
        layout = QHBoxLayout()
        layout.addWidget(self.display)
        self.setLayout(layout)
        self.setWindowTitle(self.get_plugin_title())

        # output capture
        stdout_capture, stderr_capture = WriteToSignal(), WriteToSignal()
        stdout_capture.sig_write_received.connect(self.display.appendNotice)
        stderr_capture.sig_write_received.connect(self.display.appendError)
        self.stdout, self.stderr = stdout_capture, stderr_capture

    def get_plugin_title(self):
        return "Messages"

    def read_user_settings(self, qsettings):
        self.display.readSettings(qsettings)

    def register_plugin(self, menu=None):
        self.display.attachLoggingChannel(DEFAULT_LOG_PRIORITY)
        self._capture_stdout_and_stderr()
        self.main.add_dockwidget(self)

    def _capture_stdout_and_stderr(self):
        sys.stdout = self.stdout
        sys.stderr = self.stderr
