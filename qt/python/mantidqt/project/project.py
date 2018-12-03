# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import os
import glob
from qtpy.QtWidgets import QFileDialog, QMessageBox

from mantid import logger
from mantid.api import AnalysisDataService, AnalysisDataServiceObserver
from mantidqt.io import open_a_file_dialog
from mantidqt.project.projectloader import ProjectLoader
from mantidqt.project.projectsaver import ProjectSaver


class Project(AnalysisDataServiceObserver):
    def __init__(self, project_save_name):
        # Has the project been saved
        self.saved = False

        # Last save locations
        self.last_project_location = None

        self.project_save_name = project_save_name
        self.ads_observer = AnalysisDataServiceObserver()
        self.ads_observer.observeAll(True)

    def save(self):
        if self.last_project_location is None:
            self.save_project_as()
        else:
            # Clear directory before saving to remove old workspaces
            files = glob.glob(self.last_project_location + '/.*')
            for f in files:
                try:
                    os.remove(f)
                except OSError as e:
                    logger.debug("Whilst cleaning project directory error was thrown: " + e)
            # Actually save
            workspaces_to_save = AnalysisDataService.getObjectNames()
            project_saver = ProjectSaver(self.project_save_name)
            project_saver.save_project(directory=self.last_project_location, workspace_to_save=workspaces_to_save,
                                       interfaces_to_save=None)
            self.saved = True

    def save_as(self):
        directory = None
        # Check if it exists
        first_pass = True
        while first_pass or (not os.path.exists(directory) and os.path.exists(directory + "mantidsave.project")):
            first_pass = False
            directory = open_a_file_dialog(accept_mode=QFileDialog.AcceptSave, file_mode=QFileDialog.DirectoryOnly)
            if directory is None:
                # Cancel close dialogs
                return

        # todo: get a list of workspaces but to be implemented on GUI implementation
        self.last_project_location = directory
        workspaces_to_save = AnalysisDataService.getObjectNames()
        project_saver = ProjectSaver(self.project_save_name)
        project_saver.save_project(directory=directory, workspace_to_save=workspaces_to_save, interfaces_to_save=None)
        self.saved = True

    def load(self):
        directory = None
        # Check if it exists
        first_pass = True
        while first_pass or not os.path.isdir(directory):
            first_pass = False
            directory = open_a_file_dialog(accept_mode=QFileDialog.AcceptOpen, file_mode=QFileDialog.DirectoryOnly)
            if directory is None:
                # Cancel close dialogs
                return
        project_loader = ProjectLoader(self.project_save_name)
        project_loader.load_project(directory)
        self.last_project_location = directory

    def offer_save(self, parent):
        """
        :param parent: QWidget; Parent of the QMessageBox that is popped up
        :return: Bool; Returns false if no save needed/save complete. Returns True if need to cancel closing.
        """
        result = QMessageBox.question(parent, 'Unsaved Project', "The project is currently unsaved would you like to "
                                      "save before closing?", QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                                      QMessageBox.Yes)
        if result == QMessageBox.Yes:
            self.save()
        elif result == QMessageBox.Cancel:
            return True
        # else
        return False

    def modified_project(self):
        if not self.saved:
            return
        self.saved = False

    def anyChangeHandle(self):
        self.modified_project()
