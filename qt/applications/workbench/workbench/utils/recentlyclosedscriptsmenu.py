# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.


from qtpy.QtWidgets import QMenu, QAction
from os.path import join, exists
from mantid import ConfigService


RECENT_SCRIPT_MAX_NUMBER = 10
CACHE_FILE_NAME = "recent_script_file"
MENU_CACHE_LOCATION = join(ConfigService.getAppDataDirectory(), CACHE_FILE_NAME)


class RecentlyClosedScriptsMenu(QMenu):
    def __init__(self):
        super(RecentlyClosedScriptsMenu, self).__init__()
        self.setTitle("Open Recent Scripts")
        self.aboutToShow.connect(self.repopulate_menu)
        self.showing = False
        self.addAction(QAction("Show me please!!!!"))  # Remove live

        def set_showing_false():
            self.showing = False

        self.aboutToHide.connect(set_showing_false)

    def repopulate_menu(self):
        if not self.showing:
            self.populate_menu()
            self.showing = True

    def populate_menu(self):
        # Check cache is present or don't do anything.
        valid_scripts = []
        if exists(MENU_CACHE_LOCATION):
            with open(MENU_CACHE_LOCATION) as cache:
                lines = [line.rstrip('\n') for line in cache]
                for line in lines:
                    if exists(line):
                        valid_scripts.append(line)

        if len(valid_scripts) > 0:
            for script_path in valid_scripts:
                script_name = script_path if len(script_path) > 20 else "..." + script_path[20:]
                newAction = QAction(script_name)
                newAction.triggered.connect(self.openScript, script_path)
                self.addAction(newAction)
            self.setEnabled(True)
        else:
            self.addAction(QAction("No recently closed scripts found"))

    def openScript(self, path):
        pass