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

import workspaceloader
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
    def __init__(self, project_load_name):
        self.project_reader = ProjectReader(project_load_name)
        self.workspace_loader = workspaceloader.WorkspaceLoader()

    def load_project(self, directory):
        # Read project
        self.project_reader.read_project(directory)

        # Load in the workspaces
        self.workspace_loader.load_workspaces(directory=directory)
        return _confirm_all_workspaces_loaded(workspaces_to_confirm=self.project_reader.workspace_names)


class ProjectReader(object):
    def __init__(self, project_load_name):
        self.workspace_names = None
        self.interfaces_dicts = None
        self.plot_dicts = None
        self.project_load_name = project_load_name

    def read_project(self, directory):
        f = open(directory + "/" + self.project_load_name)
        json_data = json.load(f)
        self.workspace_names = json_data["workspaces"]
        self.interfaces_dicts = json_data["interfaces"]
