#  This file is part of the mantid workbench.
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
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""Provides a widget to wrap common behaviour for all plugins"""
from __future__ import absolute_import

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDockWidget, QMainWindow, QWidget


class PluginWidget(QWidget):

    ALLOWED_AREAS = Qt.AllDockWidgetAreas
    LOCATION = Qt.LeftDockWidgetArea
    FEATURES = QDockWidget.DockWidgetClosable | \
        QDockWidget.DockWidgetFloatable | QDockWidget.DockWidgetMovable

    def __init__(self, main_window):
        QWidget.__init__(self, main_window)
        assert isinstance(main_window, QMainWindow)

        self.dockwidget = None
        self.main = main_window

# ----------------- Plugin API --------------------

    def app_closing(self):
        raise NotImplementedError()

    def get_plugin_title(self):
        raise NotImplementedError()

    def read_user_settings(self, qsettings):
        """Called by the main window to ask the plugin to
        load user configuration"""
        raise NotImplementedError()

    def register_plugin(self, menu=None):
        """Called by the parent widget/window and should
        perform any setup required to use the plugin.
        Supply an optional menu to fill with actions
        """
        raise NotImplementedError()

# ----------------- Plugin behaviour ------------------

    def create_dockwidget(self):
        """Creates a QDockWidget suitable for wrapping
        this plugin"""
        dock = QDockWidget(self.get_plugin_title(), self.main)
        dock.setObjectName(self.__class__.__name__+"_dockwidget")
        dock.setAllowedAreas(self.ALLOWED_AREAS)
        dock.setFeatures(self.FEATURES)
        dock.setWidget(self)
        self.dockwidget = dock
        return dock, self.LOCATION

    def toggle_view(self, checked):
        """Toggle view visible or hidden"""
        if checked:
            self.dockwidget.show()
            self.dockwidget.raise_()
        else:
            self.dockwidget.hide()
