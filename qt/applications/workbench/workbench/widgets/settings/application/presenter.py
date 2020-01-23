# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
#
#
from __future__ import absolute_import, unicode_literals

from mantid.kernel import ConfigService
from workbench.config import CONF
from workbench.widgets.settings.application.view import ApplicationSettingsView

from qtpy.QtCore import Qt
from qtpy.QtGui import QFontDatabase
from qtpy.QtWidgets import QFontDialog


class ApplicationSettings(object):
    """
    Presenter of the application settings section. It handles all changes to options
    within the section, and updates the ConfigService and workbench CONF accordingly.

    If new options are added to the application settings, their events when changed should
    be handled here.
    """

    FONT = "MainWindow/font"

    def __init__(self, parent, view=None):
        self.view = view if view else ApplicationSettingsView(parent, self)
        #self.font_dialog = None
        self.parent = parent
        #self.load_current_setting_values()
        self.setup_connections()

    def load_current_setting_values(self):
        if CONF.has(self.FONT):
            font = CONF.get(self.FONT)
        else:
            font = self.parent.font().toString()
        font_name = font.split(',')[0]
        font_index = self.view.main_font.findText(font_name)
        if font_index != -1:
            self.view.main_font.setCurrentIndex(font_index)

    def setup_connections(self):
        self.view.main_font.clicked.connect(self.action_main_font_button_clicked)

    def action_main_font_button_clicked(self):
        font_dialog = QFontDialog(self.parent.font(), self.parent)
        font_dialog.open()
        font_dialog.fontSelected().connect(self.action_main_font_changed)


    def action_main_font_changed(self, font):
        CONF.set(self.FONT, font.toString())
