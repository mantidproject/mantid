# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=F0401
from __future__ import (absolute_import, division, print_function)
from mantid.api import Workspace, AnalysisDataService, WorkspaceGroup
from mantid.simpleapi import RenameWorkspace, GroupWorkspaces


def add_directory_structure(dirs):
    """
    create the nested WorkspaceGroup structure in the ADS specified by the
    stored directory attribute.
    dirs = ["dir1", "dir2"] eg. ['Muon Data', 'MUSR72105', 'MUSR72105 Raw Data']
    """
    if not dirs:
        return
    if len(dirs) > len(set(dirs)):
        raise ValueError("Group names must be unique")

    for directory in dirs:
        if not AnalysisDataService.doesExist(directory):
            workspace_group = WorkspaceGroup()
            AnalysisDataService.addOrReplace(directory, workspace_group)
        elif not isinstance(AnalysisDataService.retrieve(directory), WorkspaceGroup):
            AnalysisDataService.remove(directory)
            workspace_group = WorkspaceGroup()
            AnalysisDataService.addOrReplace(directory, workspace_group)
        else:
            # exists and is a workspace group
            pass

    # Create the nested group structure in the ADS
    previous_dir = ""
    for i, directory in enumerate(dirs):
        if i == 0:
            previous_dir = directory
            continue
        if not AnalysisDataService.retrieve(previous_dir).__contains__(directory):
            AnalysisDataService.retrieve(previous_dir).add(directory)
        previous_dir = directory


def _add_workspace_to_group(group_name, workspace_name):
    if AnalysisDataService.doesExist(group_name):
        workspaces_to_group = AnalysisDataService.retrieve(group_name).getNames()
    else:
        workspaces_to_group = []
    workspaces_to_group.append(workspace_name)
    GroupWorkspaces(InputWorkspaces=workspaces_to_group, OutputWorkspace=group_name)


class MuonWorkspaceWrapper(object):
    """
    A wrapper around a single workspace for use with MuonAnalysis.

    A single instance of the wrapped workspace exists, but it is either in the ADS or stored in a
    class attribute. Either way, a single property "workspace" will always return it; with the
    show() and hide() methods storing/retrieving it from the the ADS. The show() attribute takes a
    name, which is a string using a folder-like syntax to describe the workspaces location within a
    nested structure of group workspaces

    so for example name = "group1/group2/workspace" would store the workspace in the ADS inside
    group2, which is inside group1. The "/" character is used to separate folders in the name as in
    the example.

    A basic muon workspace which is either the workspace or the name of the workspace in the ADS
    """

    def __init__(self, workspace):
        self._is_in_ads = False
        self._workspace = None
        self._directory_structure = ""
        self._workspace_name = ""

        if isinstance(workspace, Workspace):
            self.workspace = workspace
        else:
            self.name = workspace
            self._is_in_ads = True

    def __str__(self):
        return "MuonWorkspaceWrapper Object \n" \
               "In ADS         : {}\n" \
               "Name           : {}\n" \
               "Directory      : {}\n" \
               "Workspace Type : {}\n".format(self._is_in_ads,
                                              self._workspace_name,
                                              self._directory_structure,
                                              type(self._workspace))

    @property
    def is_hidden(self):
        """Is the workspace hidden (i.e. not in the ADS)."""
        return not self._is_in_ads

    @property
    def name(self):
        """The current name of the workspace."""
        return self._directory_structure + '/' + self._workspace_name

    @property
    def workspace_name(self):
        return self._workspace_name

    @property
    def workspace(self):
        """The Workspace object."""
        if not self.is_hidden:
            return AnalysisDataService.retrieve(self._workspace_name)
        else:
            return self._workspace

    @name.setter
    def name(self, full_name):
        new_workspace_name = full_name.split("/")[-1]
        if not new_workspace_name:
            return

        if not self.is_hidden:
            if len(new_workspace_name) > 0 and self._workspace_name != str(new_workspace_name):
                RenameWorkspace(InputWorkspace=self._workspace_name, OutputWorkspace=new_workspace_name)

        self._directory_structure = "/".join(full_name.split("/")[0:-1])
        self._workspace_name = new_workspace_name

    @workspace.setter
    def workspace(self, value):
        if not self.is_hidden:
            if AnalysisDataService.doesExist(self._workspace_name):
                AnalysisDataService.remove(self._workspace_name)
            self._is_in_ads = False
        if isinstance(value, Workspace):
            self._workspace = value
        else:
            raise AttributeError("Attempting to set object of type {}, must be"
                                 " a Mantid Workspace type".format(type(value)))

    def show(self, name=''):
        """
        Show the workspace in the ADS inside the WorkspaceGroup structure specified in name
        name = dirs/../dirs/workspace_name
        """

        if not name:
            return

        self.name = str(name)

        # if not self.is_hidden:
        #     return

        if len(self.name) > 0: #and self.is_hidden:
            # add workspace to ADS
            if self.is_hidden:
                AnalysisDataService.addOrReplace(self._workspace_name, self.workspace)

            self._add_to_appropriate_groups()

            self._workspace = None
            self._is_in_ads = True
        else:
            raise ValueError("Cannot store workspace in ADS with name : ",
                             str(name))

    def _add_to_appropriate_groups(self):
        if not self._directory_structure:
            return
        workspace_list = reversed(self.name.split('/')[1:])
        group_workspace_list = reversed(self.name.split('/')[:-1])
        for workspace, workspace_group in zip(workspace_list, group_workspace_list):
            _add_workspace_to_group(workspace_group, workspace)

    def hide(self):
        """
        Remove the workspace from the ADS and store it in the class instance
        """
        if AnalysisDataService.doesExist(self._workspace_name):
            self._workspace = AnalysisDataService.retrieve(self._workspace_name)
            AnalysisDataService.remove(self._workspace_name)

        self._is_in_ads = False

    def add_directory_structure(self):
        """
        create the nested WorkspaceGroup structure in the ADS specified by the
        stored _directory attribute.
        """
        dirs = self._directory_structure.split("/")
        add_directory_structure(dirs)
