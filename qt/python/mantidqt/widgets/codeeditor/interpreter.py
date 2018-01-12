#  This file is part of the mantidqt package
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
#   GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# 3rd party imports
from qtpy.QtCore import QObject
from qtpy.QtWidgets import QStatusBar, QVBoxLayout, QWidget

# local imports
from mantidqt.widgets.codeeditor.editor import CodeEditor
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution

IDLE_STATUS_MSG = "Status: Idle"

RUNNING_STATUS_MSG = "Status: Running"


class PythonFileInterpreter(QWidget):

    def __init__(self, parent=None):
        """
        :param parent: A parent QWidget
        """
        super(PythonFileInterpreter, self).__init__(parent)

        # layout
        self.editor = CodeEditor("Python", self)
        self.status = QStatusBar(self)
        layout = QVBoxLayout()
        layout.addWidget(self.editor)
        layout.addWidget(self.status)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        # presenter
        self._presenter = PythonFileInterpreterPresenter(self, PythonCodeExecution())

    def execute_all_async(self):
        self._presenter.req_execute_all_async()

    def set_editor_readonly(self, ro):
        self.editor.setReadOnly(ro)

    def set_status_message(self, msg):
        self.status.showMessage(msg)


class PythonFileInterpreterPresenter(QObject):
    """Presenter part of MVP to control actions on the editor"""

    def __init__(self, view, model):
        super(PythonFileInterpreterPresenter, self).__init__()
        self.view = view
        self.model = model

        # connect signals
        self.model.sig_exec_success.connect(self.on_exec_success)
        self.model.sig_exec_error.connect(self.on_exec_error)
        self.model.sig_exec_progress.connect(self.view.editor.updateProgressMarker)

        # starts idle
        self.view.set_status_message(IDLE_STATUS_MSG)

    def req_execute_all_async(self):
        text = self.view.editor.text()
        if not text:
            return
        self.view.set_editor_readonly(True)
        self.view.set_status_message(RUNNING_STATUS_MSG)
        return self.model.execute_async(text)

    def on_exec_success(self):
        self.view.set_editor_readonly(False)
        self.view.set_status_message(IDLE_STATUS_MSG)

    def on_exec_error(self, task_error):
        if isinstance(task_error.exception, SyntaxError):
            lineno = task_error.exception.lineno
        else:
            lineno = task_error.stack_entries[-1][1]
        self.view.editor.updateProgressMarker(lineno, True)
        self.view.set_editor_readonly(False)
        self.view.set_status_message(IDLE_STATUS_MSG)
