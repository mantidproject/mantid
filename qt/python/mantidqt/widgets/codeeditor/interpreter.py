# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import (absolute_import, unicode_literals)

import os.path
import sys
import traceback

from qtpy.QtCore import QObject, Qt, Signal
from qtpy.QtGui import QColor, QFont, QFontMetrics
from qtpy.QtWidgets import QFileDialog, QMessageBox, QStatusBar, QVBoxLayout, QWidget

from mantidqt.io import open_a_file_dialog
from mantidqt.widgets.codeeditor.editor import CodeEditor
from mantidqt.widgets.codeeditor.errorformatter import ErrorFormatter
from mantidqt.widgets.codeeditor.execution import PythonCodeExecution
from mantidqt.widgets.embedded_find_replace_dialog.presenter import EmbeddedFindReplaceDialog

IDLE_STATUS_MSG = "Status: Idle."
LAST_JOB_MSG_TEMPLATE = "Last job completed {} at {} in {:.3f}s"
RUNNING_STATUS_MSG = "Status: Running"
ABORTED_STATUS_MSG = "Status: Aborted"

# Editor
CURRENTLINE_BKGD_COLOR = QColor(247, 236, 248)
TAB_WIDTH = 4
TAB_CHAR = '\t'
SPACE_CHAR = " "


class EditorIO(object):

    def __init__(self, editor, confirm_on_exit=True):
        self.editor = editor
        self.confirm_on_exit = confirm_on_exit

    def ask_for_filename(self):
        filename = open_a_file_dialog(parent=self.editor, default_suffix=".py", file_filter="Python Files (*.py)",
                                      accept_mode=QFileDialog.AcceptSave, file_mode=QFileDialog.AnyFile)
        if filename is not None and os.path.isdir(filename):
            # Set value to None as, we do not want to be saving a directory, it is possible to receive a directory
            filename = None
        return filename

    def save_if_required(self, prompt_for_confirm=True, force_save=False):
        """
        Save the editor's contents to a file. The function has the following options:
        - if prompt_for_confirmation is True -> then show the yes/no dialog
        - if force_save is True, and prompt_for_confirmation is False -> then save the file anyway
        - if prompt_for_confirmation and force_save are both False -> then do NOT save the file, discard all changes

        :param prompt_for_confirmation: If this is True, then the user will be prompted with a yes/no dialog to
                                        decide whether to save or discard the file.
                                        If this parameter is True, force_save will be ignored!
        :param force_save: If this is True, then if the user is NOT being prompted, the file will be saved anyway!
                           This is used for the File > Save Script (Ctrl + S) action.
        :returns: True if either saving was successful or no save was requested. Returns False if
        the operation should be cancelled
        """
        if prompt_for_confirm:
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
        elif force_save:
            return self.write()
        else:
            # pretend the user clicked No on the dialog
            return True

    def write(self, save_as=None):
        if save_as is not None:
            filename = save_as
        else:
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
    sig_progress = Signal(int)
    sig_exec_error = Signal(object)
    sig_exec_success = Signal(object)

    def __init__(self, font=None, content=None, filename=None,
                 parent=None):
        """
        :param font: A reference to the font to be used by the editor. If not supplied use the system default
        :param content: An optional string of content to pass to the editor
        :param filename: The file path where the content was read.
        :param parent: An optional parent QWidget
        """
        super(PythonFileInterpreter, self).__init__(parent)
        self.parent = parent

        # layout
        font = font if font is not None else QFont()
        self.editor = CodeEditor("AlternateCSPython", font, self)
        self.find_replace_dialog = None
        self.find_replace_dialog_shown = False
        self.status = QStatusBar(self)
        self.layout = QVBoxLayout()
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.addWidget(self.editor)
        self.layout.addWidget(self.status)
        self.setLayout(self.layout)
        self._setup_editor(content, filename)

        self._presenter = PythonFileInterpreterPresenter(self, PythonCodeExecution(content))

        self.editor.modificationChanged.connect(self.sig_editor_modified)
        self.editor.fileNameChanged.connect(self.sig_filename_modified)

        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # Connect the model signals to the view's signals so they can be accessed from outside the MVP
        self._presenter.model.sig_exec_progress.connect(self.sig_progress)
        self._presenter.model.sig_exec_error.connect(self.sig_exec_error)
        self._presenter.model.sig_exec_success.connect(self.sig_exec_success)

    def closeEvent(self, event):
        self.deleteLater()
        if self.find_replace_dialog:
            self.find_replace_dialog.close()
        super(PythonFileInterpreter, self).closeEvent(event)

    def show_find_replace_dialog(self):
        if self.find_replace_dialog is None:
            self.find_replace_dialog = EmbeddedFindReplaceDialog(self, self.editor)
            self.layout.insertWidget(0, self.find_replace_dialog.view)

        self.find_replace_dialog.show()

    def hide_find_replace_dialog(self):
        if self.find_replace_dialog is not None:
            self.find_replace_dialog.hide()

    @property
    def filename(self):
        return self.editor.fileName()

    def confirm_close(self):
        """Confirm the widget can be closed. If the editor contents are modified then
        a user can interject and cancel closing.

        :return: True if closing was considered successful, false otherwise
        """
        return self.save(prompt_for_confirmation=self.parent.confirm_on_save)

    def abort(self):
        self._presenter.req_abort()

    def execute_async(self, ignore_selection=False):
        self._presenter.req_execute_async(ignore_selection)

    def execute_async_blocking(self):
        self._presenter.req_execute_async_blocking()

    def save(self, prompt_for_confirmation=False, force_save=False):
        if self.editor.isModified():
            io = EditorIO(self.editor)
            return io.save_if_required(prompt_for_confirmation, force_save)
        else:
            return True

    def save_as(self):
        io = EditorIO(self.editor)
        new_filename = io.ask_for_filename()
        if new_filename:
            return io.write(save_as=new_filename), new_filename
        else:
            return False, None

    def set_editor_readonly(self, ro):
        self.editor.setReadOnly(ro)

    def set_status_message(self, msg):
        self.status.showMessage(msg)

    def replace_tabs_with_spaces(self):
        self.replace_text(TAB_CHAR, SPACE_CHAR * TAB_WIDTH)

    def replace_text(self, match_text, replace_text):
        if self.editor.selectedText() == '':
            self.editor.selectAll()
        new_text = self.editor.selectedText().replace(match_text, replace_text)
        self.editor.replaceSelectedText(new_text)

    def replace_spaces_with_tabs(self):
        self.replace_text(SPACE_CHAR * TAB_WIDTH, TAB_CHAR)

    def set_whitespace_visible(self):
        self.editor.setWhitespaceVisibility(CodeEditor.WsVisible)

    def set_whitespace_invisible(self):
        self.editor.setWhitespaceVisibility(CodeEditor.WsInvisible)

    def toggle_comment(self):
        selection_idxs = self._get_selection_idxs()
        self._expand_selection(selection_idxs)
        # Note selection indices to restore highlighting later
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

    def _setup_editor(self, default_content, filename):
        editor = self.editor

        # Clear default QsciScintilla key bindings that we want to allow
        # to be users of this class
        self.clear_key_binding("Ctrl+/")

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

    def clear_key_binding(self, key_str):
        """Clear a keyboard shortcut bound to a Scintilla command"""
        self.editor.clearKeyBinding(key_str)

    # "private" api
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

    def _get_selection_idxs(self):
        if self.editor.selectedText() == '':
            cursor_pos = list(self.editor.getCursorPosition())
            return cursor_pos + cursor_pos
        else:
            return list(self.editor.getSelection())

    def _expand_selection(self, selection_idxs):
        """
        Expands selection from first character of first selected line to
        last character of the last selected line.
        :param selection_idxs: Length 4 list, e.g. [row0, col0, row1, col1]
        """
        line_end_pos = len(self.editor.text().split('\n')[selection_idxs[2]].rstrip())
        line_selection_idxs = [selection_idxs[0], 0,
                               selection_idxs[2], line_end_pos]
        self.editor.setSelection(*line_selection_idxs)


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
            self.view.set_status_message(ABORTED_STATUS_MSG)

    def req_execute_async(self, ignore_selection):
        self._req_execute_impl(blocking=False, ignore_selection=ignore_selection)

    def req_execute_async_blocking(self):
        self._req_execute_impl(blocking=True)

    def _req_execute_impl(self, blocking, ignore_selection=False):
        if self.is_executing:
            return
        code_str, self._code_start_offset = self._get_code_for_execution(ignore_selection)
        if not code_str:
            return
        self.is_executing = True
        self.view.set_editor_readonly(True)
        self.view.set_status_message(RUNNING_STATUS_MSG)
        return self.model.execute_async(code_str, self.view.filename, blocking)

    def _get_code_for_execution(self, ignore_selection):
        editor = self.view.editor
        if not ignore_selection and editor.hasSelectedText():
            code_str = editor.selectedText()
            line_from, _, _, _ = editor.getSelection()
        else:
            # run everything in the file
            code_str = editor.text()
            line_from = 0
        return code_str, line_from

    def _on_exec_success(self, task_result):
        self.view.editor.updateCompletionAPI(self.model.generate_calltips())
        self._finish(success=True, task_result=task_result)

    def _on_exec_error(self, task_error):
        exc_type, exc_value, exc_stack = task_error.exc_type, task_error.exc_value, task_error.stack
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
        return IDLE_STATUS_MSG + ' ' + LAST_JOB_MSG_TEMPLATE.format(status, timestamp, elapsed_time)

    def _on_progress_update(self, lineno):
        """Update progress on the view taking into account if a selection of code is
        running"""
        self.view.editor.updateProgressMarker(lineno + self._code_start_offset, False)
