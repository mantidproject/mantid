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
import re
from qtpy.QtWidgets import QFileDialog, QMessageBox

from mantid.api import AnalysisDataService, AnalysisDataServiceObserver
from mantidqt.io import open_a_file_dialog
from mantidqt.project.projectloader import ProjectLoader
from mantidqt.project.projectsaver import ProjectSaver


class Project(AnalysisDataServiceObserver):
    def __init__(self, globalfiguremanager_instance):
        super(Project, self).__init__()
        # Has the project been saved, to Access this call .saved
        self.__saved = True

        # Last save locations
        self.last_project_location = None

        self.observeAll(True)

        self.project_file_ext = ".mtdproj"

        self.plot_gfm = globalfiguremanager_instance

    def save(self):
        if self.last_project_location is None:
            self.save_as()
        else:
            # Clear unused workspaces
            self._clear_unused_workspaces(self.last_project_location)
            # Actually save
            workspaces_to_save = AnalysisDataService.getObjectNames()
            plots_to_save = self.plot_gfm.figs
            project_saver = ProjectSaver(self.project_file_ext)
            project_saver.save_project(directory=self.last_project_location, workspace_to_save=workspaces_to_save,
                                       interfaces_to_save=None, plots_to_save=plots_to_save)
            self.__saved = True

    def save_as(self):
        directory = self._get_directory_finder(accept_mode=QFileDialog.AcceptSave)

        # If none then the user cancelled
        if directory is None:
            return

        # todo: get a list of workspaces but to be implemented on GUI implementation
        self.last_project_location = directory
        workspaces_to_save = AnalysisDataService.getObjectNames()
        plots_to_save = self.plot_gfm.figs
        project_saver = ProjectSaver(self.project_file_ext)
        project_saver.save_project(directory=directory, workspace_to_save=workspaces_to_save, interfaces_to_save=None,
                                   plots_to_save=plots_to_save)
        self.__saved = True

    @staticmethod
    def _get_directory_finder(accept_mode):
        directory = None
        # Check if it exists
        first_pass = True
        while first_pass or (not os.path.exists(directory) and os.path.exists(directory + (os.path.basename(directory)
                                                                                           + ".mtdproj"))):
            first_pass = False
            directory = open_a_file_dialog(accept_mode=accept_mode, file_mode=QFileDialog.DirectoryOnly)
            if directory is None:
                # Cancel close dialogs
                return

        return directory

    def load(self):
        directory = self._get_directory_finder(accept_mode=QFileDialog.AcceptOpen)

        # If none then the user cancelled
        if directory is None:
            return

        project_loader = ProjectLoader(self.project_file_ext)
        project_loader.load_project(directory)
        self.last_project_location = directory

    def offer_save(self, parent):
        """
        :param parent: QWidget; Parent of the QMessageBox that is popped up
        :return: Bool; Returns false if no save needed/save complete. Returns True if need to cancel closing.
        """
        # If the current project is saved then return and don't do anything
        if self.__saved:
            return

        result = self._offer_save_message_box(parent)

        if result == QMessageBox.Yes:
            self.save()
        elif result == QMessageBox.Cancel:
            return True
        # else
        return False

    @staticmethod
    def _offer_save_message_box(parent):
        return QMessageBox.question(parent, 'Unsaved Project', "The project is currently unsaved would you like to "
                                    "save before closing?", QMessageBox.Yes | QMessageBox.No | QMessageBox.Cancel,
                                    QMessageBox.Yes)

    def modified_project(self):
        self.__saved = False

    def anyChangeHandle(self):
        self.modified_project()

    def __get_saved(self):
        return self.__saved

    saved = property(__get_saved)

    @staticmethod
    def _clear_unused_workspaces(path):
        files_to_remove = []
        list_dir = os.listdir(path)
        current_workspaces = AnalysisDataService.getObjectNames()
        for item in list_dir:
            # Don't count or check files that do not end in .nxs, and check that they are not in current workspaces
            # without the .nxs
            value = re.search('.nxs$', item)
            if bool(value):
                workspace_name = item.replace(".nxs", "")
                if workspace_name not in current_workspaces:
                    files_to_remove.append(item)

        # Actually remove them
        for filename in files_to_remove:
            os.remove(path + "/" + filename)
