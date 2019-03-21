# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

# system imports
import os.path as osp

# third-party library imports
from qtpy.QtWidgets import QVBoxLayout

# local package imports
from mantid.kernel import logger
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter
from ..config.fonts import text_font
from ..plugins.base import PluginWidget

# from mantidqt.utils.qt import toQSettings when readSettings/writeSettings are implemented


# Initial content
DEFAULT_CONTENT = """# The following line helps with future compatibility with Python 3
# print must now be used as a function, e.g print('Hello','World')
from __future__ import (absolute_import, division, print_function, unicode_literals)

# import mantid algorithms, numpy and matplotlib
from mantid.simpleapi import *

import matplotlib.pyplot as plt

import numpy as np
"""
# Accepted extensions for drag-and-drop to editor
ACCEPTED_FILE_EXTENSIONS = ['.py', '.pyw']
# QSettings key for session tabs
TAB_SETTINGS_KEY = "Editors/SessionTabs"


class MultiFileEditor(PluginWidget):
    """
    Provides the container for the widget containing the CodeEditors in the Workbench
    """

    def __init__(self, parent):
        super(MultiFileEditor, self).__init__(parent)

        # layout
        self.editors = MultiPythonFileInterpreter(font=text_font(),
                                                  default_content=DEFAULT_CONTENT,
                                                  parent=self)
        layout = QVBoxLayout()
        layout.addWidget(self.editors)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        self.setAcceptDrops(True)

        # attributes
        self.tabs_open_on_closing = None

    def load_settings_from_config(self, config):
        self.editors.load_settings_from_config(config)

    def execute_current_async(self):
        """
        Executes the selection in the currently active code editor.
        This is used by MainWindow to execute a file after opening it.

        """
        return self.editors.execute_current_async()

    def execute_async(self):
        """
        Executes _everything_ in currently active code editor.
        :return:
        """
        return self.editors.execute_async()

    def restore_session_tabs(self, session_tabs):
        self.open_files_in_new_tabs(session_tabs, startup=True)
        self.editors.close_tab(0)  # close default empty tab

    # ----------- Plugin API --------------------

    def app_closing(self):
        """
        Tries to close all editors
        :return: True if editors can be closed, false if cancelled
        """
        self.tabs_open_on_closing = self.editors.tab_filepaths
        return self.editors.close_all()

    def dragEnterEvent(self, event):
        data = event.mimeData()
        if data.hasText() and data.hasUrls():
            filepaths = [url.toLocalFile() for url in data.urls()]
            for filepath in filepaths:
                if osp.splitext(filepath)[1] in ACCEPTED_FILE_EXTENSIONS:
                    event.acceptProposedAction()

    def dropEvent(self, event):
        data = event.mimeData()
        for url in data.urls():
            filepath = url.toLocalFile()
            if osp.splitext(filepath)[1] in ACCEPTED_FILE_EXTENSIONS:
                try:
                    self.open_file_in_new_tab(filepath)
                except IOError as io_error:
                    logger.warning("Could not load file:\n  '{}'"
                                   "".format(io_error))

    def get_plugin_title(self):
        return "Editor"

    def readSettings(self, settings):
        try:
            prev_session_tabs = settings.get(TAB_SETTINGS_KEY)
        except KeyError:
            return
        self.restore_session_tabs(prev_session_tabs)

    def writeSettings(self, settings):
        settings.set(TAB_SETTINGS_KEY, set(self.tabs_open_on_closing))

    def register_plugin(self):
        self.main.add_dockwidget(self)

    # ----------- Plugin Behaviour --------------------

    def open_file_in_new_tab(self, filepath, startup=False):
        return self.editors.open_file_in_new_tab(filepath, startup)

    def open_files_in_new_tabs(self, filepaths, startup=False):
        for filepath in filepaths:
            try:
                self.open_file_in_new_tab(filepath, startup)
            except IOError as io_error:
                logger.warning("Could not load file:\n  {}"
                               "".format(io_error))

    def save_current_file(self):
        self.editors.save_current_file()

    def save_current_file_as(self):
        self.editors.save_current_file_as()
