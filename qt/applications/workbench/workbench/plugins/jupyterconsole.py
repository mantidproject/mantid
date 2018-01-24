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
from mantidqt.widgets.jupyterconsole import InProcessJupyterConsole
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from workbench.plugins.base import PluginWidget


class JupyterConsole(PluginWidget):
    """Provides an in-process Jupyter Qt-based console"""

    def __init__(self, parent):
        super(JupyterConsole, self).__init__(parent)

        # layout
        self.console = InProcessJupyterConsole(self)
        layout = QVBoxLayout()
        layout.addWidget(self.console)
        self.setLayout(layout)

# ----------------- Plugin API --------------------

    def get_plugin_title(self):
        return "IPython"

    def read_user_settings(self, _):
        pass

    def register_plugin(self, menu=None):
        self.main.add_dockwidget(self)
