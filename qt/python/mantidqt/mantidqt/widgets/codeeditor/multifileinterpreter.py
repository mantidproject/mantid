# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
# std imports
import os.path as osp
from os import linesep

# 3rd party imports
from qtpy.QtCore import Qt, Slot, Signal
from qtpy.QtWidgets import QVBoxLayout, QWidget

# local imports
from mantidqt.widgets.codeeditor.interpreter import PythonFileInterpreter
from mantidqt.widgets.codeeditor.scriptcompatibility import add_mantid_api_import, mantid_api_import_needed
from mantidqt.widgets.codeeditor.tab_widget.codeeditor_tab_view import CodeEditorTabWidget

NEW_TAB_TITLE = 'New'
MODIFIED_MARKER = '*'


class MultiPythonFileInterpreter(QWidget):
    """Provides a tabbed widget for editing multiple files"""

    sig_code_exec_start = Signal(str)
    sig_file_name_changed = Signal(str, str)
    sig_current_tab_changed = Signal(str)
    sig_tab_closed = Signal(str)

    def __init__(self, font=None, default_content=None, parent=None):
        """

        :param font: An optional font to override the default editor font
        :param default_content: str, if provided this will populate any new editor that is created
        :param parent: An optional parent widget
        """
        super(MultiPythonFileInterpreter, self).__init__(parent)

        # attributes
        self.default_content = default_content
        self.default_font = font
        self.prev_session_tabs = None
        self.whitespace_visible = False
        self.setAttribute(Qt.WA_DeleteOnClose, True)

        # widget setup
        layout = QVBoxLayout(self)
        self._tabs = CodeEditorTabWidget(self)
        self._tabs.currentChanged.connect(self._emit_current_tab_changed)
        layout.addWidget(self._tabs)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        self.zoom_level = 0

        # add a single editor by default
        self.append_new_editor()

        # setting defaults
        self.confirm_on_save = True

    def _tab_title_and_tooltip(self, filename):
        """Create labels for the tab title and tooltip from a filename"""
        if filename is None:
            title = NEW_TAB_TITLE
            i = 1
            while title in self.stripped_tab_titles:
                title = "{} ({})".format(NEW_TAB_TITLE, i)
                i += 1
            return title, title
        else:
            return osp.basename(filename), filename

    @property
    def stripped_tab_titles(self):
        tab_text = [self._tabs.tabText(i) for i in range(self.editor_count)]
        tab_text = [txt.rstrip('*') for txt in tab_text]
        # Some DEs (such as KDE) will automatically assign keyboard shortcuts using the Qt & annotation
        # see Qt Docs - qtabwidget#addTab
        tab_text = [txt.replace('&', '') for txt in tab_text]
        return tab_text

    def closeEvent(self, event):
        self.deleteLater()
        super(MultiPythonFileInterpreter, self).closeEvent(event)

    def load_settings_from_config(self, config):
        self.confirm_on_save = config.get('project', 'prompt_save_editor_modified')

    @property
    def editor_count(self):
        return self._tabs.count()

    @property
    def tab_filepaths(self):
        file_paths = []
        for idx in range(self.editor_count):
            file_path = self.editor_at(idx).filename
            if file_path:
                file_paths.append(file_path)
        return file_paths

    def append_new_editor(self, font=None, content=None, filename=None):
        """
        Appends a new editor the tabbed widget
        :param font: A reference to the font to be used by the editor. If None is given
        then self.default_font is used
        :param content: An optional string containing content to be placed
        into the editor on opening. If None then self.default_content is used
        :param filename: An optional string containing the filename of the editor
        if applicable.
        :return:
        """
        if content is None:
            content = self.default_content
        if font is None:
            font = self.default_font

        if self.editor_count > 0:
            # If there are other tabs open the same zoom level
            # as these is used.
            current_zoom = self._tabs.widget(0).editor.getZoom()
        else:
            # Otherwise the zoom level of the last tab closed is used
            # Or the default (0) if this is the very first tab
            current_zoom = self.zoom_level

        interpreter = PythonFileInterpreter(font, content, filename=filename,
                                            parent=self)

        interpreter.editor.zoomTo(current_zoom)

        if self.whitespace_visible:
            interpreter.set_whitespace_visible()

        # monitor future modifications
        interpreter.sig_editor_modified.connect(self.mark_current_tab_modified)
        interpreter.sig_filename_modified.connect(self.on_filename_modified)
        interpreter.editor.textZoomedIn.connect(self.zoom_in_all_tabs)
        interpreter.editor.textZoomedOut.connect(self.zoom_out_all_tabs)

        tab_title, tab_tooltip = self._tab_title_and_tooltip(filename)
        tab_idx = self._tabs.addTab(interpreter, tab_title)
        self._tabs.setTabToolTip(tab_idx, tab_tooltip)
        self._tabs.setCurrentIndex(tab_idx)

        # set the cursor to the last line and give the new editor focus
        interpreter.editor.setFocus()
        if content is not None:
            line_count = content.count(linesep)
            interpreter.editor.setCursorPosition(line_count,0)
        return tab_idx

    def abort_current(self):
        """Request that that the current execution be cancelled"""
        self.current_editor().abort()

    @Slot()
    def abort_all(self):
        """Request that all executing tabs are cancelled"""
        for ii in range(0, len(self._tabs)):
            editor = self.editor_at(ii)
            editor.abort()

    def close_all(self):
        """
        Close all tabs
        :return: True if all tabs are closed, False if cancelled
        """
        for idx in reversed(range(self.editor_count)):
            if not self.close_tab(idx, allow_zero_tabs=True):
                return False

        return True

    def close_tab(self, idx, allow_zero_tabs=False):
        """
        Close the tab at the given index.
        :param idx: The tab index
        :param allow_zero_tabs: If True then closing the last tab does not add a new empty tab.
        :return: True if tab is to be closed, False if cancelled
        """

        if idx >= self.editor_count:
            return True
        # Make the current tab active so that it is clear what you
        # are being prompted to save
        self._tabs.setCurrentIndex(idx)
        if self.current_editor().confirm_close():

            # If the last editor tab is being closed, its zoom level
            # is saved for the new tab which opens automatically.
            if self.editor_count == 1:
                self.zoom_level = self.current_editor().editor.getZoom()

            widget = self.editor_at(idx)
            filename = self.editor_at(idx).filename
            # note: this does not close the widget, that is why we manually close it
            self._tabs.removeTab(idx)
            widget.close()
        else:
            return False

        if (not allow_zero_tabs) and self.editor_count == 0:
            self.append_new_editor()

        if filename is not None:
            self.sig_tab_closed.emit(filename)
        else:
            self.sig_tab_closed.emit("")

        return True

    def current_editor(self):
        return self._tabs.currentWidget()

    def editor_at(self, idx):
        """Return the editor at the given index. Must be in range"""
        return self._tabs.widget(idx)

    def _emit_current_tab_changed(self, index):
        if index == -1:
            self.sig_current_tab_changed.emit("")
        else:
            self.sig_current_tab_changed.emit(self.current_tab_filename)

    def _emit_code_exec_start(self):
        """Emit signal that code execution has started"""
        if not self.current_editor().filename:
            filename = self._tabs.tabText(self._tabs.currentIndex()).rstrip('*')
            self.sig_code_exec_start.emit(filename)
        else:
            self.sig_code_exec_start.emit(self.current_editor().filename)

    @property
    def current_tab_filename(self):
        if not self.current_editor().filename:
            return self._tabs.tabText(self._tabs.currentIndex()).rstrip('*')
        return self.current_editor().filename

    def execute_current_async(self):
        """
        Execute content of the current file. If a selection is active
        then only this portion of code is executed, this is completed asynchronously
        """
        self._emit_code_exec_start()
        return self.current_editor().execute_async()

    def execute_async(self):
        """
        Execute ALL the content in the current file.
        Selection is ignored.
        This is completed asynchronously.
        """
        self._emit_code_exec_start()
        return self.current_editor().execute_async(ignore_selection=True)

    @Slot()
    def execute_current_async_blocking(self):
        """
        Execute content of the current file. If a selection is active
        then only this portion of code is executed, completed asynchronously
        which blocks calling thread.
        """
        self._emit_code_exec_start()
        self.current_editor().execute_async_blocking()

    def mark_current_tab_modified(self, modified):
        """Update the current tab title to indicate that the
        content has been modified"""
        self.mark_tab_modified(self._tabs.currentIndex(), modified)

    def mark_tab_modified(self, idx, modified):
        """Update the tab title to indicate that the
        content has been modified or not"""
        title_cur = self._tabs.tabText(idx)
        if modified:
            if not title_cur.endswith(MODIFIED_MARKER):
                title_new = title_cur + MODIFIED_MARKER
            else:
                title_new = title_cur
        else:
            if title_cur.endswith(MODIFIED_MARKER):
                title_new = title_cur.rstrip('*')
            else:
                title_new = title_cur
        self._tabs.setTabText(idx, title_new)

    def on_filename_modified(self, filename):
        old_filename = self._tabs.tabToolTip(self._tabs.currentIndex()).rstrip('*')
        if not filename:
            filename = self._tabs.tabText(self._tabs.currentIndex()).rstrip('*')
        self.sig_file_name_changed.emit(old_filename, filename)
        title, tooltip = self._tab_title_and_tooltip(filename)
        idx_cur = self._tabs.currentIndex()
        self._tabs.setTabText(idx_cur, title)
        self._tabs.setTabToolTip(idx_cur, tooltip)

    @Slot(str)
    def open_file_in_new_tab(self, filepath, startup=False):
        """Open the existing file in a new tab in the editor

        :param filepath: A path to an existing file
        :param startup: Flag for if function is being called on startup
        """
        with open(filepath, 'r') as code_file:
            content = code_file.read()

        self.append_new_editor(content=content, filename=filepath)
        if startup is False and mantid_api_import_needed(content) is True:
            add_mantid_api_import(self.current_editor().editor, content)

    def open_files_in_new_tabs(self, filepaths):
        for filepath in filepaths:
            self.open_file_in_new_tab(filepath)

    def plus_button_clicked(self, _):
        """Add a new tab when the plus button is clicked"""
        self.append_new_editor()

    def restore_session_tabs(self):
        if self.prev_session_tabs is not None:
            try:
                self.open_files_in_new_tabs(self.prev_session_tabs)
            except IOError:
                pass
            self.close_tab(0)  # close default empty script

    def save_current_file(self):
        """Save the current file"""
        self.current_editor().save(force_save=True)

    def save_current_file_as(self):
        previous_filename = self.current_editor().filename
        saved, filename = self.current_editor().save_as()
        if saved:
            self.current_editor().close()
            self.open_file_in_new_tab(filename)
            if previous_filename:
                self.sig_file_name_changed.emit(previous_filename, filename)

    def spaces_to_tabs_current(self):
        self.current_editor().replace_spaces_with_tabs()

    def tabs_to_spaces_current(self):
        self.current_editor().replace_tabs_with_spaces()

    def toggle_comment_current(self):
        self.current_editor().toggle_comment()

    def toggle_find_replace_dialog(self):
        self.current_editor().show_find_replace_dialog()

    def toggle_whitespace_visible_all(self):
        if self.whitespace_visible:
            for idx in range(self.editor_count):
                self.editor_at(idx).set_whitespace_invisible()
            self.whitespace_visible = False
        else:
            for idx in range(self.editor_count):
                self.editor_at(idx).set_whitespace_visible()
            self.whitespace_visible = True

    def zoom_in_all_tabs(self):
        current_tab_index = self._tabs.currentIndex()

        for i in range(self.editor_count):
            if i == current_tab_index:
                continue

            self.editor_at(i).editor.zoomIn()

    def zoom_out_all_tabs(self):
        current_tab_index = self._tabs.currentIndex()

        for i in range(self.editor_count):
            if i == current_tab_index:
                continue

            self.editor_at(i).editor.zoomOut()
