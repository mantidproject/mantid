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
from qtpy.QtCore import QObject, Signal
from qtpy.QtGui import QColor, QFontMetrics
from qtpy.QtWidgets import QFileDialog, QMessageBox, QStatusBar, QVBoxLayout, QWidget

# local imports
from mantidqt.widgets.codeeditor.editor import CodeEditor
from mantidqt.widgets.codeeditor.errorformatter import ErrorFormatter
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution

# Status messages
IDLE_STATUS_MSG = "Status: Idle."
LAST_JOB_MSG_TEMPLATE = "Last job completed {} in {:.3f}s"
RUNNING_STATUS_MSG = "Status: Running"

# Editor
CURRENTLINE_BKGD_COLOR = QColor(247, 236, 248)
TAB_WIDTH = 4


class EditorIO(object):

    def __init__(self, editor):
        self.editor = editor

    def ask_for_filename(self):
        filename, _ = QFileDialog.getSaveFileName(self.editor, "Choose filename...")
        return filename

    def save_if_required(self, confirm=True):
        """Asks the user if the contents should be saved.

        :param confirm: If True then show a confirmation dialog first to check we should save
        :returns: True if either saving was successful or no save was requested. Returns False if
        the operation should be cancelled
        """
        if confirm:
            button = QMessageBox.question(self.editor, "",
                                          "Save changes to document before closing?",
                                          buttons=(QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel),
                                          defaultButton=QMessageBox.Cancel)
            if button == QMessageBox.Yes:
                return self.write()
            elif button == QMessageBox.No:
                return True
            else:
                # Cancelled
                return False
        else:
            return self.write()

    def write(self):
        filename = self.editor.fileName()
        if not filename:
            filename = self.ask_for_filename()
            if not filename:
                return False
            self.editor.setFileName(filename)

        try:
            with open(filename, 'w') as f:
                f.write(self.editor.text())
            self.editor.setModified(False)
        except IOError as exc:
            QMessageBox.warning(self.editor, "",
                                "Error while saving '{}': {}".format(filename, str(exc)))
            return False
        else:
            return True


class PythonFileInterpreter(QWidget):
    sig_editor_modified = Signal(bool)
    sig_filename_modified = Signal(str)

    def __init__(self, content=None, filename=None,
                 parent=None):
        """
        :param content: An optional string of content to pass to the editor
        :param filename: The file path where the content was read.
        :param parent: An optional parent QWidget
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
        self._setup_editor(content, filename)

        self._presenter = PythonFileInterpreterPresenter(self,
                                                         PythonCodeExecution(content))

        self.editor.modificationChanged.connect(self.sig_editor_modified)
        self.editor.fileNameChanged.connect(self.sig_filename_modified)

    @property
    def filename(self):
        return self.editor.fileName()

    def confirm_close(self):
        """Confirm the widget can be closed. If the editor contents are modified then
        a user can interject and cancel closing.

        :return: True if closing was considered successful, false otherwise
        """
        return self.save(confirm=True)

    def abort(self):
        self._presenter.req_abort()

    def execute_async(self):
        self._presenter.req_execute_async()

    def save(self, confirm=False):
        if self.editor.isModified():
            io = EditorIO(self.editor)
            return io.save_if_required(confirm)
        else:
            return True

    def set_editor_readonly(self, ro):
        self.editor.setReadOnly(ro)

    def set_status_message(self, msg):
        self.status.showMessage(msg)

    def _setup_editor(self, default_content, filename):
        editor = self.editor

        # use tabs not spaces for indentation
        editor.setIndentationsUseTabs(False)
        editor.setTabWidth(TAB_WIDTH)

        # show current editing line but in a softer color
        editor.setCaretLineBackgroundColor(CURRENTLINE_BKGD_COLOR)
        editor.setCaretLineVisible(True)

        # set a margin large enough for sensible file sizes < 1000 lines
        # and the progress marker
        font_metrics = QFontMetrics(self.font())
        editor.setMarginWidth(1, font_metrics.averageCharWidth()*3 + 12)

        # fill with content if supplied and set source filename
        if default_content is not None:
            editor.setText(default_content)
        if filename is not None:
            editor.setFileName(filename)
        # Default content does not count as a modification
        editor.setModified(False)

        editor.enableAutoCompletion(CodeEditor.AcsAll)


class PythonFileInterpreterPresenter(QObject):
    """Presenter part of MVP to control actions on the editor"""

    def __init__(self, view, model):
        super(PythonFileInterpreterPresenter, self).__init__()
        # attributes
        self.view = view
        self.model = model
        # offset of executing code from start of the file
        self._code_start_offset = 0
        self._is_executing = False
        self._error_formatter = ErrorFormatter()

        # If startup code was executed then populate autocomplete
        self.view.editor.updateCompletionAPI(self.model.generate_calltips())

        # connect signals
        self.model.sig_exec_success.connect(self._on_exec_success)
        self.model.sig_exec_error.connect(self._on_exec_error)
        self.model.sig_exec_progress.connect(self._on_progress_update)

        # starts idle
        self.view.set_status_message(IDLE_STATUS_MSG)

    @property
    def is_executing(self):
        return self._is_executing

    @is_executing.setter
    def is_executing(self, value):
        self._is_executing = value

    def req_abort(self):
        if self.is_executing:
            self.model.abort()

    def req_execute_async(self):
        if self.is_executing:
            return
        self.is_executing = True
        code_str, self._code_start_offset = self._get_code_for_execution()
        if not code_str:
            return
        self.view.set_editor_readonly(True)
        self.view.set_status_message(RUNNING_STATUS_MSG)
        return self.model.execute_async(code_str, self.view.filename)

    def _get_code_for_execution(self):
        editor = self.view.editor
        if editor.hasSelectedText():
            code_str = editor.selectedText()
            line_from, _, _, _ = editor.getSelection()
        else:
            code_str = editor.text()
            line_from = 0
        # Pad code out with empty lines so that reported line numbers
        # do not have to be adjusted
        padded = '\n'*line_from + code_str
        return padded, line_from

    def _on_exec_success(self, task_result):
        self.view.editor.updateCompletionAPI(self.model.generate_calltips())
        self._finish(success=True, elapsed_time=task_result.elapsed_time)

    def _on_exec_error(self, task_error):
        exc_type, exc_value, exc_stack = task_error.exc_type, task_error.exc_value, \
                                         task_error.stack
        if hasattr(exc_value, 'lineno'):
            lineno = exc_value.lineno
        elif exc_stack is not None:
            lineno = exc_stack[-1][1]
        else:
            lineno = -1
        sys.stderr.write(self._error_formatter.format(exc_type, exc_value, exc_stack) + '\n')
        self.view.editor.updateProgressMarker(lineno, True)
        self._finish(success=False, elapsed_time=task_error.elapsed_time)

    def _finish(self, success, elapsed_time):
        status = 'successfully' if success else 'with errors'
        self.view.set_status_message(self._create_status_msg(status,
                                                             elapsed_time))
        self.view.set_editor_readonly(False)
        self.is_executing = False

    def _create_status_msg(self, status, elapsed_time):
        return IDLE_STATUS_MSG + ' ' + \
               LAST_JOB_MSG_TEMPLATE.format(status,
                                            elapsed_time)

    def _on_progress_update(self, lineno):
        """Update progress on the view taking into account if a selection of code is
        running"""
        self.view.editor.updateProgressMarker(lineno + self._code_start_offset,
                                              False)
