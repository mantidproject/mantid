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
from qtpy.QtCore import QObject, Signal
from qtpy.QtWidgets import QStatusBar, QTabWidget, QVBoxLayout, QWidget

# local imports
from mantidqt.utils.qt import import_qtlib
from .execution import PythonCodeExecution

DEFAULT_EDITOR_LANGUAGE = "Python"

EXECUTION_CLS = PythonCodeExecution

IDLE_STATUS_MSG = "Status: Idle"

RUNNING_STATUS_MSG = "Status: Running"

# Import single-file editor from C++ wrapping
CodeEditor = import_qtlib('_widgetscore', 'mantidqt.widgets', 'ScriptEditor')


class ExecutableCodeEditor(QWidget):

    def __init__(self, language=DEFAULT_EDITOR_LANGUAGE, parent=None):
        """

        :param language: Language for syntax highlighting
        :param user_globals: Dictionary for global context of execution.
        :param user_locals: Dictionary for local context of execution
        :param parent: A parent QWidget
        """
        super(ExecutableCodeEditor, self).__init__(parent)

        # layout
        self._editor = CodeEditor(language, self)
        self._status = QStatusBar(self)
        layout = QVBoxLayout()
        layout.addWidget(self._editor)
        layout.addWidget(self._status)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        self.presenter = ExecutableCodeEditorPresenter(self)

    def execute_all_async(self):
        self.presenter.req_execute_all_async()

    def set_editor_readonly(self, ro):
        self._editor.setReadOnly(ro)

    def set_status_message(self, msg):
        self._status.showMessage(msg)


class ExecutableCodeEditorPresenter(QObject):
    """Presenter part of MVP to control actions on the editor"""

    def __init__(self, view):
        super(ExecutableCodeEditorPresenter, self).__init__()
        self.view = view
        self.model = PythonCodeExecution(success_cb=self._on_exec_success)

        self.view.set_status_message(IDLE_STATUS_MSG)

    def req_execute_all_async(self):
        text = self.view.text()
        if not text:
            return
        self.view.set_editor_read_only()
        self.view.set_status_message(RUNNING_STATUS_MSG)
        self.model.execute_async(text)

    def _on_exec_success(self):
        self.view.set_status_message(IDLE_STATUS_MSG)


class MultiFileCodeEditor(QWidget):
    """Provides a tabbed widget for editing multiple files"""

    def __init__(self, parent=None):
        super(MultiFileCodeEditor, self).__init__(parent)

        # layout
        self.editors = QTabWidget(self)
        self.editors.setMovable(True)
        layout = QVBoxLayout()
        layout.addWidget(self.editors)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        # add a single editor by default
        self.append_new_editor(DEFAULT_EDITOR_LANGUAGE)

    @property
    def editor_count(self):
        return self.editors.count()

    def append_new_editor(self, language):
        title = "New"
        self.editors.addTab(ExecutableCodeEditor(language, self.editors), title)
