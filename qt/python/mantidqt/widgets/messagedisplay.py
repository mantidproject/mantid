# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

from qtpy.QtWidgets import QAction
from qtpy.QtGui import QFont

from mantidqt.utils.qt import import_qt

SHOW_SCRIPT_OUTPUT_KEY = "MessageDisplayShowScriptOutput"
SHOW_FRAMEWORK_OUTPUT_KEY = "MessageDisplayShowFrameworkOutput"

MessageDisplay_cpp = import_qt('.._common', 'mantidqt.widgets', 'MessageDisplay')


class Priority:
    Fatal = 2
    Error = 3
    Warning = 4
    Notice = 5
    Information = 6
    Debug = 7


class MessageDisplay(MessageDisplay_cpp):

    def __init__(self, font=QFont(), parent=None):
        super(MessageDisplay, self).__init__(font, parent)

        # We need to disconnect from the C++ base class's slot to avoid
        # calling the base class's showContextMenu method as well
        self.getTextEdit().customContextMenuRequested.disconnect()
        self.getTextEdit().customContextMenuRequested.connect(self.showContextMenu)

        self.script_name = None

    def readSettings(self, qsettings):
        super(MessageDisplay, self).readSettings(qsettings)
        self.setShowFrameworkOutput(qsettings.value(SHOW_FRAMEWORK_OUTPUT_KEY, True))
        self.setShowScriptOutput(qsettings.value(SHOW_SCRIPT_OUTPUT_KEY, True))

    def writeSettings(self, qsettings):
        super(MessageDisplay, self).writeSettings(qsettings)
        qsettings.setValue(SHOW_FRAMEWORK_OUTPUT_KEY, self.showFrameworkOutput())
        qsettings.setValue(SHOW_SCRIPT_OUTPUT_KEY, self.showScriptOutput())

    def generateContextMenu(self):
        qmenu = super(MessageDisplay, self).generateContextMenu()
        filter_menu = qmenu.addMenu("&Filter by")

        framework_action = QAction('Framework Output', filter_menu)
        framework_action.triggered.connect(self.toggle_filter_framework_output)
        framework_action.setCheckable(True)
        framework_action.setChecked(self.showFrameworkOutput())
        filter_menu.addAction(framework_action)

        all_script_action = QAction('All Script Output', filter_menu)
        all_script_action.triggered.connect(self.toggle_filter_all_script_output)
        all_script_action.setCheckable(True)
        all_script_action.setChecked(self.showAllScriptOutput())
        filter_menu.addAction(all_script_action)
        return qmenu

    def showContextMenu(self, q_position):
        self.generateContextMenu().exec_(self.mapToGlobal(q_position))

    def append_script_error(self, txt):
        """
        Append the given message to the window, marking the message as
        output from a Python script with "Error" priority. This function
        is hooked into stderr.
        """
        self.appendPython(txt, Priority.Error, self.script_name)

    def append_script_notice(self, txt):
        """
        Append the given message to the window, marking the message as
        output from a Python script with "Notice" priority. This
        function is hooked into stdout.
        """
        self.appendPython(txt, Priority.Notice, self.script_name)

    def toggle_filter_framework_output(self):
        self.setShowFrameworkOutput(not self.showFrameworkOutput())
        self.filterMessages()

    def toggle_filter_all_script_output(self):
        self.setShowAllScriptOutput(not self.showAllScriptOutput())
        self.filterMessages()
