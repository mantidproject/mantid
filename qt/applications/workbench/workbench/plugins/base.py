# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""Provides a widget to wrap common behaviour for all plugins"""

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDockWidget, QMainWindow, QWidget


class PluginWidget(QWidget):
    ALLOWED_AREAS = Qt.AllDockWidgetAreas
    LOCATION = Qt.LeftDockWidgetArea
    FEATURES = QDockWidget.DockWidgetClosable | QDockWidget.DockWidgetFloatable | QDockWidget.DockWidgetMovable

    def __init__(self, main_window):
        QWidget.__init__(self, main_window)
        assert isinstance(main_window, QMainWindow)

        self.dockwidget = None
        self.main = main_window
        self.settings_read = False

    # ----------------- Plugin API --------------------

    def app_closing(self):
        raise NotImplementedError()

    def get_plugin_title(self):
        raise NotImplementedError()

    def readSettings(self, qsettings):
        """Load user configuration"""
        raise NotImplementedError()

    def readSettingsIfNotDone(self, qsettings):
        """Called by the main window to ask the plugin to
        load user configuration"""
        if not self.settings_read:
            self.readSettings(qsettings)
            self.settings_read = True

    def writeSettings(self, qsettings):
        """Called by the main window to ask the plugin to
        save user configuration"""
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
        dock.setObjectName(self.__class__.__name__ + "_dockwidget")
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
