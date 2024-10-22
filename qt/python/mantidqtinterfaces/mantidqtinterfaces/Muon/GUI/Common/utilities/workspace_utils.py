# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from dataclasses import dataclass

from mantid.api import Workspace
from mantid.simpleapi import CloneWorkspace


@dataclass
class StaticWorkspaceWrapper:
    """
    A wrapper around a workspace name and its corresponding workspace. It should be used when we want to store a
    workspace which should never change.

    The workspace is deliberately decoupled from the ADS to avoid a potential expiration of its lifetime when
    removed from the ADS. It also ensures that the state of a workspace at a given point in time is saved. If the
    workspace was a shared pointer to an object in the ADS, then the values in the workspace can be overwritten by
    a simple ADS replacement event - this wrapper helps avoid this.

    Only a copy of the workspace should be returned by this wrapper if you want to modify the workspace, or add it to
    the ADS. This is to avoid the shared pointer to the workspace in this class becoming a shared pointer to a workspace
    that can be overwritten in the ADS.
    """

    workspace_name: str
    workspace: Workspace

    def __init__(self, workspace_name: str, workspace: Workspace):
        self.workspace_name = workspace_name
        self.workspace = self._clone_workspace(workspace)

    def workspace_copy(self) -> Workspace:
        """This should be used if you plan on modifying the returned workspace, or placing it in the ADS."""
        return self._clone_workspace(self.workspace)

    @staticmethod
    def _clone_workspace(workspace: Workspace) -> Workspace:
        """Clones the provided workspace and ensures the output is decoupled from the ADS."""
        copy = CloneWorkspace(InputWorkspace=workspace, StoreInADS=False, EnableLogging=False)
        return copy
