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
from mantidqt.widgets.manageuserdirectories import ManageUserDirectories
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget


class ManageUserDirectoriesWidget(PluginWidget):
    """Provides the widget for managing user directories"""

    def __init__(self, parent):
        super(ManageUserDirectoriesWidget, self).__init__(parent)

        # layout
        self.widget = ManageUserDirectories(self)
        layout = QVBoxLayout()
        layout.addWidget(self.widget)
        self.setLayout(layout)

# ----------------- Plugin API --------------------

    def get_plugin_title(self):
        return "Manage User Directories"

    def read_user_settings(self, _):
        pass

    def register_plugin(self, menu=None):
        # Does not need to be reqistered as it is not a dock widget
        pass
