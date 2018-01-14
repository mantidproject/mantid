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

# std imports
import sys

# 3rd party imports
from qtpy.QtCore import QObject
from qtpy.QtGui import QColor, QFontMetrics
from qtpy.QtWidgets import QStatusBar, QVBoxLayout, QWidget

# local imports
from mantidqt.widgets.codeeditor.editor import CodeEditor
from mantidqt.widgets.codeeditor.errorformatter import ErrorFormatter
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution

# Status messages
IDLE_STATUS_MSG = "Status: Idle"
RUNNING_STATUS_MSG = "Status: Running"

# Editor colors
CURRENTLINE_BKGD_COLOR = QColor(247, 236, 248)


class PythonFileInterpreter(QWidget):

    def __init__(self, default_content=None, parent=None):
        """
        :param parent: A parent QWidget
        """
        super(PythonFileInterpreter, self).__init__(parent)

        # layout
        self.editor = CodeEditor("AlternateCSPythonLexer", self)
        self.status = QStatusBar(self)
        layout = QVBoxLayout()
        layout.addWidget(self.editor)
        layout.addWidget(self.status)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        self._setup_editor(default_content)

        # presenter
        self._presenter = PythonFileInterpreterPresenter(self, PythonCodeExecution())

    def execute_async(self):
        self._presenter.req_execute_async()

    def set_editor_readonly(self, ro):
        self.editor.setReadOnly(ro)

    def set_status_message(self, msg):
        self.status.showMessage(msg)

    def _setup_editor(self, default_content):
        editor = self.editor

        # show current editing line but in a softer color
        editor.setCaretLineBackgroundColor(CURRENTLINE_BKGD_COLOR)
        editor.setCaretLineVisible(True)

        # set a margin large enough for sensible file sizes < 1000 lines
        # and the progress marker
        font_metrics = QFontMetrics(self.font())
        editor.setMarginWidth(1, font_metrics.averageCharWidth()*3 + 12)

        # fill with content if supplied
        if default_content is not None:
            editor.setText(default_content)


class PythonFileInterpreterPresenter(QObject):
    """Presenter part of MVP to control actions on the editor"""

    def __init__(self, view, model):
        super(PythonFileInterpreterPresenter, self).__init__()
        # attributes
        self.view = view
        self.model = model
        # offset of executing code from start of the file
        self._code_start_offset = 0
        self._error_formatter = ErrorFormatter()

        # connect signals
        self.model.sig_exec_success.connect(self._on_exec_success)
        self.model.sig_exec_error.connect(self._on_exec_error)
        self.model.sig_exec_progress.connect(self._on_progress_update)

        # starts idle
        self.view.set_status_message(IDLE_STATUS_MSG)

    def req_execute_async(self):
        code_str, self._code_start_offset = self._get_code_for_execution()
        if not code_str:
            return
        self.view.set_editor_readonly(True)
        self.view.set_status_message(RUNNING_STATUS_MSG)
        return self.model.execute_async(code_str)

    def _get_code_for_execution(self):
        editor = self.view.editor
        if editor.hasSelectedText():
            code_str = editor.selectedText()
            line_from, _, _, _ = editor.getSelection()
        else:
            code_str = editor.text()
            line_from = 0
        return code_str, line_from

    def _on_exec_success(self):
        self.view.set_editor_readonly(False)
        self.view.set_status_message(IDLE_STATUS_MSG)

    def _on_exec_error(self, task_error):
        exc_type, exc_value, exc_stack = task_error.exc_type, task_error.exc_value, \
                                         task_error.stack
        if isinstance(exc_value, SyntaxError):
            lineno = exc_value.lineno
        else:
            lineno = exc_stack[-1][1]

        self.view.editor.updateProgressMarker(lineno, True)
        sys.stderr.write(self._error_formatter.format(exc_type, exc_value, exc_stack) + '\n')
        self.view.set_editor_readonly(False)
        self.view.set_status_message(IDLE_STATUS_MSG)

    def _on_progress_update(self, lineno):
        """Update progress on the view taking into account if a selection of code is
        running"""
        self.view.editor.updateProgressMarker(lineno + self._code_start_offset,
                                              False)
