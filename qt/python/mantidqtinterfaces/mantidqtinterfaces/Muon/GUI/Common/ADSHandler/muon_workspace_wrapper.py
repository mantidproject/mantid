# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=F0401
from mantid.api import Workspace, AnalysisDataService
from mantid.simpleapi import RenameWorkspace


class MuonWorkspaceWrapper(object):
    """
    A wrapper around a single workspace for use with MuonAnalysis.

    A single instance of the wrapped workspace exists, but it is either in the ADS or stored in a
    class attribute. Either way, a single property "workspace" will always return it; with the
    show() and hide() methods storing/retrieving it from the ADS. The show() attribute takes a
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
        return "MuonWorkspaceWrapper Object \nIn ADS         : {}\nName           : {}\nDirectory      : {}\nWorkspace Type : {}\n".format(
            self._is_in_ads, self._workspace_name, self._directory_structure, type(self._workspace)
        )

    @property
    def is_hidden(self):
        """Is the workspace hidden (i.e. not in the ADS)."""
        return not self._is_in_ads

    @property
    def name(self):
        """The current name of the workspace."""
        directory = self._directory_structure + "/" if self._directory_structure else ""
        return directory + self._workspace_name

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
            raise AttributeError("Attempting to set object of type {}, must be a Mantid Workspace type".format(type(value)))

    def show(self, name=""):
        """
        Show the workspace in the ADS inside the WorkspaceGroup structure specified in name
        name = dirs/../dirs/workspace_name
        """
        if not name and not self.name:
            raise ValueError("Cannot store workspace in ADS with name : ", str(name))

        self.name = str(name)

        if len(self.name) > 0:
            # add workspace to ADS
            if self.is_hidden:
                AnalysisDataService.addOrReplace(self._workspace_name, self.workspace)

            self._workspace = None
            self._is_in_ads = True
        else:
            raise ValueError("Cannot store workspace in ADS with name : ", str(name))

    def hide(self):
        """
        Remove the workspace from the ADS and store it in the class instance
        """
        if AnalysisDataService.doesExist(self._workspace_name):
            self._workspace = AnalysisDataService.retrieve(self._workspace_name)
            AnalysisDataService.remove(self._workspace_name)

        self._is_in_ads = False
