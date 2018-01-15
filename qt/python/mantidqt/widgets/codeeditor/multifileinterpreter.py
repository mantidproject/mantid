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
from qtpy.QtWidgets import QTabWidget, QVBoxLayout, QWidget

# local imports
from mantidqt.widgets.codeeditor.interpreter import PythonFileInterpreter

NEW_TAB_TITLE = 'temp.py'
MODIFIED_MARKER = '*'


class MultiPythonFileInterpreter(QWidget):
    """Provides a tabbed widget for editing multiple files"""

    def __init__(self, default_content=None, parent=None):
        super(MultiPythonFileInterpreter, self).__init__(parent)

        # attributes
        self.default_content = default_content

        # layout
        self._tabs = QTabWidget(self)
        self._tabs.setMovable(True)
        layout = QVBoxLayout()
        layout.addWidget(self._tabs)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        # add a single editor by default
        self.append_new_editor()

    @property
    def editor_count(self):
        return self._tabs.count()

    def append_new_editor(self):
        interpreter = PythonFileInterpreter(self.default_content,
                                            parent=self._tabs)
        interpreter.sig_editor_modified.connect(self.update_tab_title)
        self._tabs.addTab(interpreter, NEW_TAB_TITLE)
        self.update_tab_title(modified=True)

    def current_editor(self):
        return self._tabs.currentWidget()

    def execute_current(self):
        """Execute content of the current file. If a selection is active
        then only this portion of code is executed"""
        self.current_editor().execute_async()

    def update_tab_title(self, modified):
        """Update the current tab title to indicate that the
        content has been modified"""
        idx_cur = self._tabs.currentIndex()
        title_cur = self._tabs.tabText(idx_cur)
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
        self._tabs.setTabText(idx_cur, title_new)
