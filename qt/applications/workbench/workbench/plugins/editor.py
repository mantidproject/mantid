#    This file is part of the mantid workbench.
#
#    Copyright (C) 2017 mantidproject
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, unicode_literals)

# system imports

# third-party library imports
from mantidqt.utils.qt import add_actions, create_action
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget


class MultiFileEditor(PluginWidget):
    """Provides a tab widget for editing multiple files"""

    def __init__(self, parent):
        super(MultiFileEditor, self).__init__(parent)

        # layout
        self.editors = MultiPythonFileInterpreter(self)
        layout = QVBoxLayout()
        layout.addWidget(self.editors)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        # attributes
        self.run_action = create_action(self, "Run",
                                        on_triggered=self.execute_current)
        self.editor_actions = [self.run_action]

    # ----------- Plugin API --------------------

    def register_plugin(self, menu=None):
        self.main.add_dockwidget(self)
        add_actions(menu, self.editor_actions)

    def get_plugin_title(self):
        return "Editor"

    # ----------- Plugin behaviour --------------

    def execute_current(self):
        self.editors.execute_current()
