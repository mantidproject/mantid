# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)

import json
import os

from mantidqt.project import workspaceloader
from mantid import AnalysisDataService as ADS
from mantid import logger


def _confirm_all_workspaces_loaded(workspaces_to_confirm):
    current_workspaces = ADS.getObjectNames()
    for ws in workspaces_to_confirm:
        if ws not in current_workspaces:
            logger.warning("Project Loader was unable to load back all of project workspaces")
            return False
    return True


class ProjectLoader(object):
    def __init__(self, project_file_ext):
        self.project_reader = ProjectReader(project_file_ext)
        self.workspace_loader = workspaceloader.WorkspaceLoader()
        self.project_file_ext = project_file_ext

    def load_project(self, directory):
        """
        Will load the project in the given directory
        :param directory: String or string castable object; the directory of the project
        :return: Bool; True if all workspace loaded successfully, False if not loaded successfully.
        """
        # It can be expected that if at this point it is NoneType that it's an error
        if directory is None:
            return

        # Read project
        self.project_reader.read_project(directory)

        # Load in the workspaces
        self.workspace_loader.load_workspaces(directory=directory,
                                              workspaces_to_load=self.project_reader.workspace_names)
        return _confirm_all_workspaces_loaded(workspaces_to_confirm=self.project_reader.workspace_names)


class ProjectReader(object):
    def __init__(self, project_file_ext):
        self.workspace_names = None
        self.plot_dicts = None
        self.project_file_ext = project_file_ext

    def read_project(self, directory):
        """
        Will read the project file in from the directory that is given.
        :param directory: String or string castable object; the directory of the project
        """
        try:
            with open(os.path.join(directory, (os.path.basename(directory) + self.project_file_ext))) as f:
                json_data = json.load(f)
                self.workspace_names = json_data["workspaces"]
        except Exception:
            logger.warning("JSON project file unable to be loaded/read")
