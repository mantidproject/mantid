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

from qtpy.QtWidgets import QAction, QActionGroup
from qtpy.QtGui import QFont

from mantidqt.utils.qt import import_qt

SHOW_FRAMEWORK_OUTPUT_KEY = "MessageDisplay/ShowFrameworkOutput"
SHOW_ALL_SCRIPT_OUTPUT_KEY = "MessageDisplay/ShowAllScriptOutput"
SHOW_ACTIVE_SCRIPT_OUTPUT_KEY = "MessageDisplay/ShowActiveScriptOutput"

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

        self.last_executed_script = ""

    def readSettings(self, qsettings):
        super(MessageDisplay, self).readSettings(qsettings)
        self.setShowFrameworkOutput(qsettings.value(SHOW_FRAMEWORK_OUTPUT_KEY, True))
        self.setShowActiveScriptOutput(qsettings.value(SHOW_ACTIVE_SCRIPT_OUTPUT_KEY, True))
        self.setShowAllScriptOutput(qsettings.value(SHOW_ALL_SCRIPT_OUTPUT_KEY, False))

    def writeSettings(self, qsettings):
        super(MessageDisplay, self).writeSettings(qsettings)
        qsettings.setValue(SHOW_FRAMEWORK_OUTPUT_KEY, self.showFrameworkOutput())
        qsettings.setValue(SHOW_ALL_SCRIPT_OUTPUT_KEY, self.showAllScriptOutput())
        qsettings.setValue(SHOW_ACTIVE_SCRIPT_OUTPUT_KEY, self.showActiveScriptOutput())

    def generateContextMenu(self):
        """
        Generate the window's context menu. This first calls the base class's
        context menu generator and then extends it with the filtering options.
        """
        qmenu = super(MessageDisplay, self).generateContextMenu()
        filter_menu = qmenu.addMenu("&View")

        framework_action = QAction('Mantid Log Output', filter_menu)
        framework_action.triggered.connect(self.toggle_filter_framework_output)
        framework_action.setCheckable(True)
        framework_action.setChecked(self.showFrameworkOutput())
        filter_menu.addAction(framework_action)

        filter_menu.addSeparator()

        actions_to_group = []
        active_script_action = QAction("Active Tab Output", filter_menu)
        active_script_action.triggered.connect(self.show_active_script)
        actions_to_group.append(active_script_action)
        all_script_action = QAction('All Script Output', filter_menu)
        all_script_action.triggered.connect(self.show_all_scripts)
        actions_to_group.append(all_script_action)
        hide_all_script_action = QAction("Hide All Script Output", filter_menu)
        hide_all_script_action.triggered.connect(self.hide_all_scripts)
        actions_to_group.append(hide_all_script_action)

        action_group = QActionGroup(filter_menu)
        for action in actions_to_group:
            action_group.addAction(action)
            filter_menu.addAction(action)
            action.setCheckable(True)

        if self.showAllScriptOutput():
            all_script_action.setChecked(True)
        elif self.showActiveScriptOutput():
            active_script_action.setChecked(True)
        else:
            hide_all_script_action.setChecked(True)
        return qmenu

    def showContextMenu(self, q_position):
        self.generateContextMenu().exec_(self.mapToGlobal(q_position))

    def show_all_scripts(self):
        if not self.showAllScriptOutput():
            self.setShowAllScriptOutput(True)
            self.setShowActiveScriptOutput(False)
            self.filterMessages()

    def hide_all_scripts(self):
        if self.showActiveScriptOutput() or self.showAllScriptOutput():
            self.setShowAllScriptOutput(False)
            self.setShowActiveScriptOutput(False)
            self.filterMessages()

    def show_active_script(self):
        if not self.showActiveScriptOutput():
            self.setShowAllScriptOutput(False)
            self.setShowActiveScriptOutput(True)
            self.filterMessages()

    def toggle_filter_framework_output(self):
        self.setShowFrameworkOutput(not self.showFrameworkOutput())
        self.filterMessages()

    def append_script_error(self, txt):
        """
        Append the given message to the window, marking the message as
        output from a Python script with "Error" priority. This function
        is hooked into stderr.
        """
        self.appendPython(txt, Priority.Error, self.last_executed_script)

    def append_script_notice(self, txt):
        """
        Append the given message to the window, marking the message as
        output from a Python script with "Notice" priority. This
        function is hooked into stdout.
        """
        self.appendPython(txt, Priority.Notice, self.last_executed_script)

    def script_executing(self, script_path):
        """Slot executed when a script is executed in the Workbench."""
        self.last_executed_script = script_path

    def file_name_modified(self, old_file_name, new_file_name):
        self.filePathModified(old_file_name, new_file_name)
        if self.activeScript() == old_file_name:
            self.setActiveScript(new_file_name)

    def current_tab_changed(self, script_path):
        self.setActiveScript(script_path)
        if self.showActiveScriptOutput():
            self.filterMessages()
