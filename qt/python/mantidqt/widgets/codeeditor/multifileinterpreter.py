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


class MultiPythonFileInterpreter(QWidget):
    """Provides a tabbed widget for editing multiple files"""

    def __init__(self, default_content=None, parent=None):
        super(MultiPythonFileInterpreter, self).__init__(parent)

        # attributes
        self.default_content = default_content

        # layout
        self._editors = QTabWidget(self)
        self._editors.setMovable(True)
        layout = QVBoxLayout()
        layout.addWidget(self._editors)
        self.setLayout(layout)
        layout.setContentsMargins(0, 0, 0, 0)

        # add a single editor by default
        self.append_new_editor()

    @property
    def editor_count(self):
        return self._editors.count()

    def current_editor(self):
        return self._editors.currentWidget()

    def execute_current(self):
        """Execute content of the current file. If a selection is active
        then only this portion of code is executed"""
        self.current_editor().execute_async()

    def append_new_editor(self):
        title = "New"
        self._editors.addTab(PythonFileInterpreter(self.default_content,
                                                   parent=None),
                             title)

