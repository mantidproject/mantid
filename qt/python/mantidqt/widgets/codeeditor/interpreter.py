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

# std imports
import sys
import os.path
import traceback

# 3rd party imports
from qtpy.QtCore import QObject, Signal
from qtpy.QtGui import QColor, QFontMetrics, QKeySequence
from qtpy.QtWidgets import QMessageBox, QStatusBar, QVBoxLayout, QWidget, QFileDialog

# local imports
from mantidqt.widgets.codeeditor.editor import CodeEditor
from mantidqt.widgets.codeeditor.errorformatter import ErrorFormatter
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution
from mantidqt.io import open_a_file_dialog

# Status messages
IDLE_STATUS_MSG = "Status: Idle."
LAST_JOB_MSG_TEMPLATE = "Last job completed {} at {} in {:.3f}s"
RUNNING_STATUS_MSG = "Status: Running"

# Editor
CURRENTLINE_BKGD_COLOR = QColor(247, 236, 248)
TAB_WIDTH = 4


class EditorIO(object):

    def __init__(self, editor):
        self.editor = editor

    def ask_for_filename(self):
        filename = open_a_file_dialog(parent=self.editor, default_suffix=".py", file_filter="Python Files (*.py)",
                                      accept_mode=QFileDialog.AcceptSave, file_mode=QFileDialog.AnyFile)
        if filename is not None and os.path.isdir(filename):
            # Set value to None as, we do not want to be saving a directory, it is possible to receive a directory
            filename = None
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

        # Clear QsciScintilla key bindings that may override PyQt's bindings
        self.clear_key_binding("Ctrl+/")

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

    def replace_tabs_with_spaces(self):
        if self.editor.selectedText() == '':
            self.editor.selectAll()
        new_text = self.editor.selectedText().replace('\t', '    ')
        self.editor.replaceSelectedText(new_text)

    def replace_spaces_with_tabs(self):
        if self.editor.selectedText() == '':
            self.editor.selectAll()
        new_text = self.editor.selectedText().replace('    ', '\t')
        self.editor.replaceSelectedText(new_text)

    def set_whitespace_visible(self):
        self.editor.setWhitespaceVisibility(CodeEditor.WsVisible)

    def set_whitespace_invisible(self):
        self.editor.setWhitespaceVisibility(CodeEditor.WsInvisible)

    def clear_key_binding(self, key_str):
        """Clear a keyboard shortcut bound to a Scintilla command"""
        if not QKeySequence(key_str).toString():
            raise ValueError("Invalid key combination '%s'".format(key_str))
        self.editor.clearKeyBinding(key_str)

    def toggle_comment(self):
        if self.editor.selectedText() == '':   # If nothing selected, do nothing
            return

        # Note selection indices to restore highlighting later
        selection_idxs = list(self.editor.getSelection())

        # Expand selection from first character on start line to end char on last line
        line_end_pos = len(self.editor.text().split('\n')[selection_idxs[2]].rstrip())
        line_selection_idxs = [selection_idxs[0], 0,
                               selection_idxs[2], line_end_pos]
        self.editor.setSelection(*line_selection_idxs)
        selected_lines = self.editor.selectedText().split('\n')

        if self._are_comments(selected_lines) is True:
            toggled_lines = self._uncomment_lines(selected_lines)
            # Track deleted characters to keep highlighting consistent
            selection_idxs[1] -= 2
            selection_idxs[-1] -= 2
        else:
            toggled_lines = self._comment_lines(selected_lines)
            selection_idxs[1] += 2
            selection_idxs[-1] += 2

        # Replace lines with commented/uncommented lines
        self.editor.replaceSelectedText('\n'.join(toggled_lines))

        # Restore highlighting
        self.editor.setSelection(*selection_idxs)

    def _comment_lines(self, lines):
        for i in range(len(lines)):
            lines[i] = '# ' + lines[i]
        return lines

    def _uncomment_lines(self, lines):
        for i in range(len(lines)):
            uncommented_line = lines[i].replace('# ', '', 1)
            if uncommented_line == lines[i]:
                uncommented_line = lines[i].replace('#', '', 1)
            lines[i] = uncommented_line
        return lines

    def _are_comments(self, code_lines):
        for line in code_lines:
            if line.strip():
                if not line.strip().startswith('#'):
                    return False
        return True

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
        editor.setMarginWidth(1, font_metrics.averageCharWidth() * 3 + 20)

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
    MAX_STACKTRACE_LENGTH = 2

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
        code_str, self._code_start_offset = self._get_code_for_execution()
        if not code_str:
            return
        self.is_executing = True
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
        return code_str, line_from

    def _on_exec_success(self, task_result):
        self.view.editor.updateCompletionAPI(self.model.generate_calltips())
        self._finish(success=True, task_result=task_result)

    def _on_exec_error(self, task_error):
        exc_type, exc_value, exc_stack = task_error.exc_type, task_error.exc_value, \
                                         task_error.stack
        exc_stack = traceback.extract_tb(exc_stack)[self.MAX_STACKTRACE_LENGTH:]
        if hasattr(exc_value, 'lineno'):
            lineno = exc_value.lineno + self._code_start_offset
        elif exc_stack is not None:
            lineno = exc_stack[-1][1] + self._code_start_offset
        else:
            lineno = -1
        sys.stderr.write(self._error_formatter.format(exc_type, exc_value, exc_stack) + os.linesep)
        self.view.editor.updateProgressMarker(lineno, True)
        self._finish(success=False, task_result=task_error)

    def _finish(self, success, task_result):
        status = 'successfully' if success else 'with errors'
        status_message = self._create_status_msg(status, task_result.timestamp,
                                                 task_result.elapsed_time)
        self.view.set_status_message(status_message)
        self.view.set_editor_readonly(False)
        self.is_executing = False

    def _create_status_msg(self, status, timestamp, elapsed_time):
        return IDLE_STATUS_MSG + ' ' + \
               LAST_JOB_MSG_TEMPLATE.format(status, timestamp, elapsed_time)

    def _on_progress_update(self, lineno):
        """Update progress on the view taking into account if a selection of code is
        running"""
        self.view.editor.updateProgressMarker(lineno + self._code_start_offset,
                                              False)
