from mantid.simpleapi import CreateWorkspace
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


def create_workspace_wrapper_stub_object(name):
    workspace = CreateWorkspace([0], [0])
    return MuonWorkspaceWrapper(workspace, name)