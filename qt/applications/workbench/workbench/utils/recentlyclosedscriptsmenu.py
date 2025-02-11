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
from mantid.kernel import logger
from mantidqt.utils.qt import create_action
from workbench.config import CONF

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
        scripts = self._get_scripts_from_settings()

        if len(scripts) > 0:
            for script_path in scripts:
                script_name = self.size_path_correctly(script_path)
                new_action = create_action(
                    parent=self.mainwindow, text=script_name, on_triggered=functools.partial(self.open_script, script_path)
                )
                self.addAction(new_action)
        else:
            self.addAction(create_action(parent=self.mainwindow, text="No recently closed scripts found"))

    @staticmethod
    def size_path_correctly(path):
        return path if len(path) < 33 else "..." + path[-30:]

    def open_script(self, path):
        # Check if it exists, if it doesn't pop up small window saying sorry this doesn't exist, then remove it from
        # stack. else pass it off to the script window to open.
        if exists(path):
            self.mainwindow.editor.open_file_in_new_tab(path)
        else:
            # Remove path from script settings, then warn user.
            self.remove_script_from_settings(path)
            QMessageBox().warning(
                None,
                "That script no longer exists!",
                "Are all network drives properly mounted? or are there any network connectivity problems?",
                QMessageBox.Ok,
            )

    def remove_script_from_settings(self, path):
        scripts = self._get_scripts_from_settings()
        if path in scripts:
            scripts.remove(path)
            self._store_scripts_to_settings(scripts)

    def add_script_to_settings(self, path):
        if path is None or path == "":
            return
        scripts = self._get_scripts_from_settings()

        if path not in scripts:
            scripts.insert(0, path)

            if len(scripts) > RECENT_SCRIPT_MAX_NUMBER:
                scripts.pop()

            self._store_scripts_to_settings(scripts)

    @staticmethod
    def _get_scripts_from_settings():
        scripts = []
        try:
            scripts = CONF.get(RECENT_SCRIPTS_KEY, type=list)
        except KeyError:
            # Happens quite often and should fail silently.
            pass
        except TypeError:
            # Happens when garbage data is found in the QSettings .ini file
            logger.error("Recently Opened Scripts were lost during save, and workbench has recovered from an error.")
            CONF.set(RECENT_SCRIPTS_KEY, [])

        def sort_key(sub_list):
            return sub_list[0]

        scripts.sort(key=sort_key)
        # strip scripts of it's extra data and overwrite the list
        for index, script in enumerate(scripts):
            scripts[index] = script[1]

        return scripts

    @staticmethod
    def _store_scripts_to_settings(scripts):
        # Add an index to a tuple in the script
        for index, script in enumerate(scripts):
            scripts[index] = (index, script)

        CONF.set(RECENT_SCRIPTS_KEY, scripts)
