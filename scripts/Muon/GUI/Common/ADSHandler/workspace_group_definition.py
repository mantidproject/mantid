# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import GroupWorkspaces
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist


# A singleton metaclass, required for the WorkspaceGroupDefinition class.
class Singleton(type):
    """
    A singleton metaclass, required for the WorkspaceGroupDefinition class.
    """
    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


class WorkspaceGroupDefinition(metaclass=Singleton):
    """
    This class can have a context manager attached to it, which prevents the grouping algorithm from running
    until the desired groups and workspaces names have been added to the instance.
    The grouping algorithm will then be executed on the contents of the grouping dict.
    The grouping dict contains: {workspace_group_name: [workspaces in that group]}
    """

    def __init__(self):
        self.__saveState = False
        self.grouping = {}

    def execute_grouping(self):
        if not self.__saveState:
            self._remove_non_existing_workspaces()
            for group_name, workspace_set in self.grouping.items():
                workspace_list = list(workspace_set)
                if len(workspace_list) > 0:
                    GroupWorkspaces(InputWorkspaces=workspace_list, OutputWorkspace=group_name)

    def new_workspace_group(self, group_name):
        if group_name not in self.grouping:
            self.grouping[group_name] = set()

    def add_workspaces_to_group(self, group_name, workspace_names):
        self.new_workspace_group(group_name)
        for workspace_name in workspace_names:
            self.grouping[group_name].add(workspace_name)

    def _remove_non_existing_workspaces(self):
        for group_name, workspace_set in self.grouping.items():
            for workspace_name in list(workspace_set):
                if not check_if_workspace_exist(workspace_name):
                    self.grouping[group_name].remove(workspace_name)

    def __enter__(self):
        self.__saveState = True
        self.grouping.clear()
        return self

    def __exit__(self, type, value, traceback):
        self.__saveState = False
        self.execute_grouping()
        self.grouping.clear()
