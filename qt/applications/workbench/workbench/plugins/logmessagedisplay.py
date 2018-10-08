# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
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
ORIGINAL_STDOUT = sys.stdout
ORIGINAL_STDERR = sys.stderr


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
        self.stdout = WriteToSignal(ORIGINAL_STDOUT)
        self.stdout.sig_write_received.connect(self.display.appendNotice)
        self.stderr = WriteToSignal(ORIGINAL_STDERR)
        self.stderr.sig_write_received.connect(self.display.appendError)

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
