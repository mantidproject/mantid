# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import functools

from qtpy.QtWidgets import QMenu, QMessageBox
from os.path import join, exists
from mantid import ConfigService
from mantidqt.utils.qt import create_action
from config import CONF

RECENT_SCRIPT_MAX_NUMBER = 10
CACHE_FILE_NAME = "recent_script_file"
MENU_CACHE_LOCATION = join(ConfigService.getAppDataDirectory(), CACHE_FILE_NAME)
RECENT_SCRIPTS_KEY = "RecentScripts"


class RecentlyClosedScriptsMenu(QMenu):
    def __init__(self, mainwindow):
        super(RecentlyClosedScriptsMenu, self).__init__()
        self.setTitle("Open Recently Closed Scripts")
        self.aboutToShow.connect(self.repopulate_menu)
        self.mainwindow = mainwindow

    def repopulate_menu(self):
        self.clear()
        self.populate_menu()

    def populate_menu(self):
        # Check cache is present or don't do anything.
        scripts = self.load_scripts_from_settings()

        if len(scripts) > 0:
            for script_path in scripts:
                script_name = self.size_path_correctly(script_path)
                new_action = create_action(parent=self.mainwindow,
                                           text=script_name,
                                           on_triggered=functools.partial(self.open_script, script_path))
                self.addAction(new_action)
        else:
            self.addAction(create_action(parent=self.mainwindow,
                                         text="No recently closed scripts found"))

    @staticmethod
    def size_path_correctly(path):
        return path if len(path) < 30 else "..." + path[30:]

    def open_script(self, path):
        # Check if it exists, if it doesn't pop up small window saying sorry this doesn't exist, then remove it from
        # stack. else pass it off to the script window to open.
        if exists(path):
            self.mainwindow.editor.open_file_in_new_tab(path)
        else:
            # Remove path from script settings, then warn user.
            self.remove_script_from_settings(path)
            QMessageBox().warning(None, "That script no longer exists!",
                                  "Are all network drives properly mounted? or are there any network connectivity "
                                  "problems?",
                                  QMessageBox.Ok)

    @staticmethod
    def remove_script_from_settings(path):
        if CONF.has(RECENT_SCRIPTS_KEY):
            scripts = CONF.get(RECENT_SCRIPTS_KEY)
            if path in scripts:
                scripts.remove(path)

    @staticmethod
    def load_scripts_from_settings():
        scripts = []
        if CONF.has(RECENT_SCRIPTS_KEY):
            scripts = CONF.get(RECENT_SCRIPTS_KEY)

        # Ensure there are no duplicates
        scripts = list(set(scripts))

        return scripts

    @staticmethod
    def add_script_to_settings(path):
        if path is None or path == '':
            return
        scripts = []
        if CONF.has(RECENT_SCRIPTS_KEY):
            scripts = CONF.get(RECENT_SCRIPTS_KEY)
        scripts.insert(0, path)

        # Ensure there are no duplicates
        scripts = list(set(scripts))

        if len(scripts) > RECENT_SCRIPT_MAX_NUMBER:
            scripts.pop()

        CONF.set(RECENT_SCRIPTS_KEY, scripts)
