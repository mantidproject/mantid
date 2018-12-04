# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

from json import dump
import os

from mantidqt.project import workspacesaver
from mantid import logger


class ProjectSaver(object):
    def __init__(self, project_file_ext):
        self.project_file_ext = project_file_ext

    def save_project(self, directory, workspace_to_save=None, interfaces_to_save=None):
        """

        :param directory: String;
        :param workspace_to_save: List; of Strings that will have workspace names in it, if None will save all
        :param interfaces_to_save: List; of Strings that will have interface tags in it, if None will save all
        :return:
        """
        # Check if the directory doesn't exist
        if directory is None:
            logger.warning("Can not save to empty directory")
            return

        # Check this isn't saving a blank project file
        if workspace_to_save is None and interfaces_to_save is None:
            logger.warning("Can not save an empty project")
            return

        # Save workspaces to that location
        workspace_saver = workspacesaver.WorkspaceSaver(directory=directory)
        workspace_saver.save_workspaces(workspaces_to_save=workspace_to_save)

        # Get interface details in dicts
        dictionaries_to_save = {}

        # Pass dicts to Project Writer
        writer = ProjectWriter(dictionaries_to_save, directory, workspace_saver.get_output_list(),
                               self.project_file_ext)
        writer.write_out()


class ProjectWriter(object):
    def __init__(self, dicts, save_location, workspace_names, project_file_ext):
        """

        :param dicts:
        :param save_location:
        :param workspace_names:
        """
        self.dicts_to_save = dicts
        self.workspace_names = workspace_names
        self.directory = save_location
        self.project_file_ext = project_file_ext

    def write_out(self):
        """

        """
        # Get the JSON string versions
        workspace_interface_dict = {"workspaces": self.workspace_names, "interfaces": self.dicts_to_save}

        # Open file and save the string to it alongside the workspace_names
        file_name = self.directory + '/' + (os.path.basename(self.directory) + self.project_file_ext)
        if not os.path.isdir(self.directory):
            os.makedirs(self.directory)
        f = open(file_name, 'w+')
        dump(obj=workspace_interface_dict, fp=f)
