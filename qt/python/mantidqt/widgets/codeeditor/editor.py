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
from __future__ import (absolute_import)

# 3rd party imports
from qtpy.QtWidgets import QTabWidget, QVBoxLayout, QWidget

# local imports
from mantidqt.utils.qt import import_qtlib
from .execution import PythonCodeExecution

EDITOR_LANGUAGE = "Python"

# Import single-file editor from C++ wrapping
CodeEditor = import_qtlib('_widgetscore', 'mantidqt.widgets', 'ScriptEditor')


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

        # add a single editor by default
        self.append_new_editor(EDITOR_LANGUAGE)

    @property
    def editor_count(self):
        return self.editors.count()

    def append_new_editor(self, language):
        title = "New"
        self.editors.addTab(CodeEditor(language, self.editors), title)
