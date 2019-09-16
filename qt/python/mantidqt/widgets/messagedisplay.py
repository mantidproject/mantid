# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
from __future__ import (absolute_import, unicode_literals)

import os

from qtpy.QtWidgets import QAction, QActionGroup
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

        self.active_script = ""

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
        all_script_action.setChecked(self.showScriptOutput())
        filter_menu.addAction(all_script_action)

        # We use a QActionGroup here because of a bug where, if we hooked the
        # actions in the loop directly to `toggle_filter_by_script`, every
        # action's slot would contain the same path (the last one in the loop).
        # Using a QActionGroup, the script's path can be stored on the QAction
        # and the slot called with the path stored on the acton.
        action_group = QActionGroup(filter_menu)
        action_group.setExclusive(False)
        for script_path in sorted(self.displayedScripts()):
            script_name = os.path.basename(script_path)
            action = QAction(script_name, filter_menu)
            action.setData(script_path)
            action.setCheckable(True)
            if self.showScriptOutput() or self.inScriptsToPrint(script_path):
                action.setChecked(True)
            action_group.addAction(action)
            filter_menu.addAction(action)
        action_group.triggered.connect(
            lambda qaction: self.toggle_filter_by_script(qaction.data()))
        return qmenu

    def showContextMenu(self, q_position):
        self.generateContextMenu().exec_(self.mapToGlobal(q_position))

    def append_script_error(self, txt):
        """
        Append the given message to the window, marking the message as
        output from a Python script with "Error" priority. This function
        is hooked into stderr.
        """
        self.appendPython(txt, Priority.Error, self.active_script)

    def append_script_notice(self, txt):
        """
        Append the given message to the window, marking the message as
        output from a Python script with "Notice" priority. This
        function is hooked into stdout.
        """
        self.appendPython(txt, Priority.Notice, self.active_script)

    def toggle_filter_framework_output(self):
        self.setShowFrameworkOutput(not self.showFrameworkOutput())
        self.filterMessages()

    def toggle_filter_all_script_output(self):
        self.setShowScriptOutput(not self.showScriptOutput())
        self.filterMessages()

    def toggle_filter_by_script(self, script_path):
        print(script_path)

    def script_executing(self, script_path):
        self.active_script = script_path
        try:
            self.displayedScripts()[script_path]
        except KeyError:
            self.insertIntoDisplayedScripts(script_path, True)
