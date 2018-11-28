# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from json import dumps
from os import makedirs
from os.path import isdir

from mantidqt.project import workspacesaver
from mantid import logger


ENCODED_FILE_NAME = "mantidsave.project"


class ProjectSaver(object):
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
        writer = ProjectWriter(dictionaries_to_save, directory, workspace_saver.get_output_list())
        writer.write_out()


# Static private method to create JSON from objects and return a string
def _create_out_string_json(dictionary):
    """

    :param dictionary:
    :return:
    """
    return dumps(dictionary)


class ProjectWriter(object):
    def __init__(self, dicts, save_location, workspace_names):
        """

        :param dicts:
        :param save_location:
        :param workspace_names:
        """
        self.dicts_to_save = dicts
        self.workspace_names = workspace_names
        self.directory = save_location

    def write_out(self):
        """

        """
        # Get the JSON string versions
        workspace_interface_dict = {"workspaces": self.workspace_names, "interfaces": self.dicts_to_save}
        json_string = _create_out_string_json(workspace_interface_dict)

        # Open file and save the string to it alongside the workspace_names
        file_name = self.directory + '/' + ENCODED_FILE_NAME
        if not isdir(self.directory):
            makedirs(self.directory)
        f = open(file_name, 'w+')
        f.write(json_string)
