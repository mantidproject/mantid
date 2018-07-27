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
import os.path as osp

# 3rd party imports
from qtpy.QtCore import Qt
from qtpy.QtWidgets import (QTabWidget, QToolButton, QVBoxLayout, QWidget)

# local imports
from mantidqt.widgets.codeeditor.interpreter import PythonFileInterpreter

NEW_TAB_TITLE = 'New'
MODIFIED_MARKER = '*'


def _tab_title_and_toolip(filename):
    """Create labels for the tab title and tooltip from a filename"""
    if filename is None:
        return NEW_TAB_TITLE, NEW_TAB_TITLE
    else:
        return osp.basename(filename), filename


class MultiPythonFileInterpreter(QWidget):
    """Provides a tabbed widget for editing multiple files"""

    def __init__(self, default_content=None, parent=None):
        super(MultiPythonFileInterpreter, self).__init__(parent)

        # attributes
        self.default_content = default_content

        # widget setup
        self._tabs = self.create_tabwidget()
        layout = QVBoxLayout()
        layout.addWidget(self._tabs)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        # add a single editor by default
        self.append_new_editor()

    @property
    def editor_count(self):
        return self._tabs.count()

    def append_new_editor(self, content=None, filename=None):
        if content is None:
            content = self.default_content
        interpreter = PythonFileInterpreter(content, filename=filename,
                                            parent=self._tabs)
        # monitor future modifications
        interpreter.sig_editor_modified.connect(self.mark_current_tab_modified)
        interpreter.sig_filename_modified.connect(self.on_filename_modified)

        tab_title, tab_toolip = _tab_title_and_toolip(filename)
        tab_idx = self._tabs.addTab(interpreter, tab_title)
        self._tabs.setTabToolTip(tab_idx, tab_toolip)
        self._tabs.setCurrentIndex(tab_idx)
        return tab_idx

    def abort_current(self):
        """Request that that the current execution be cancelled"""
        self.current_editor().abort()

    def close_all(self):
        """
        Close all tabs
        :return: True if all tabs are closed, False if cancelled
        """
        for idx in reversed(range(self.editor_count)):
            if not self.close_tab(idx):
                return False

        return True

    def close_tab(self, idx):
        """
        Close the tab at the given index.
        :param idx: The tab index
        :return: True if tab is to be closed, False if cancelled
        """
        if idx >= self.editor_count:
            return True
        editor = self.editor_at(idx)
        if editor.confirm_close():
            self._tabs.removeTab(idx)
        else:
            return False

        # we never want an empty widget
        if self.editor_count == 0:
            self.append_new_editor(content=self.default_content)

        return True

    def create_tabwidget(self):
        """Create a new QTabWidget with a button to add new tabs"""
        tabs = QTabWidget(self)
        tabs.setMovable(True)
        tabs.setTabsClosable(True)
        # create a button to add new tabs
        plus_btn = QToolButton(tabs)
        plus_btn.setText('+')
        plus_btn.clicked.connect(self.plus_button_clicked)
        tabs.setCornerWidget(plus_btn, Qt.TopLeftCorner)
        tabs.tabCloseRequested.connect(self.close_tab)
        return tabs

    def current_editor(self):
        return self._tabs.currentWidget()

    def editor_at(self, idx):
        """Return the editor at the given index. Must be in range"""
        return self._tabs.widget(idx)

    def execute_current(self):
        """Execute content of the current file. If a selection is active
        then only this portion of code is executed"""
        self.current_editor().execute_async()

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
        title, tooltip = _tab_title_and_toolip(filename)
        idx_cur = self._tabs.currentIndex()
        self._tabs.setTabText(idx_cur, title)
        self._tabs.setTabToolTip(idx_cur, tooltip)

    def open_file_in_new_tab(self, filepath):
        """Open the existing file in a new tab in the editor

        :param filepath: A path to an existing file
        """
        with open(filepath, 'r') as code_file:
            content = code_file.read()
        self.append_new_editor(content=content, filename=filepath)

    def plus_button_clicked(self, _):
        """Add a new tab when the plus button is clicked"""
        self.append_new_editor()

    def save_current_file(self):
        """Save the current file"""
        self.current_editor().save()
