from mantid.simpleapi import CreateWorkspace
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.muon_group import MuonGroup


def create_workspace_wrapper_stub_object(name):
    workspace = CreateWorkspace([0], [0])
    return MuonWorkspaceWrapper(workspace, name)


def create_group_populated_by_two_workspace():
        group = MuonGroup(group_name="group1")
        counts_workspace_22222 = CreateWorkspace([0], [0])
        asymmetry_workspace_22222 = CreateWorkspace([0], [0])
        group.update_workspaces([22222], counts_workspace_22222, asymmetry_workspace_22222, False)
        group.show_raw([22222], 'counts_name_22222', 'asymmetry_name_22222')
        counts_workspace_33333 = CreateWorkspace([0], [0])
        asymmetry_workspace_33333 = CreateWorkspace([0], [0])
        group.update_workspaces([33333], counts_workspace_33333, asymmetry_workspace_33333, False)
        group.show_raw([33333], 'counts_name_33333', 'asymmetry_name_33333')

        return group


def create_group_populated_by_two_rebinned_workspaces():
    group = MuonGroup(group_name="group1")
    counts_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_22222 = CreateWorkspace([0], [0])
    group.update_workspaces([22222], counts_workspace_22222, asymmetry_workspace_22222, True)
    group.show_rebin([22222], 'counts_name_22222_rebin', 'asymmetry_name_22222_rebin')
    counts_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_33333 = CreateWorkspace([0], [0])
    group.update_workspaces([33333], counts_workspace_33333, asymmetry_workspace_33333, True)
    group.show_rebin([33333], 'counts_name_33333_rebin', 'asymmetry_name_33333_rebin')

    return group


def create_group_populated_by_two_binned_and_two_unbinned_workspaces():
    group = MuonGroup(group_name="group1")
    counts_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_22222 = CreateWorkspace([0], [0])
    group.update_workspaces([22222], counts_workspace_22222, asymmetry_workspace_22222, False)
    group.show_raw([22222], 'counts_name_22222', 'asymmetry_name_22222')
    counts_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_33333 = CreateWorkspace([0], [0])
    group.update_workspaces([33333], counts_workspace_33333, asymmetry_workspace_33333, False)
    group.show_raw([33333], 'counts_name_33333', 'asymmetry_name_33333')

    counts_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_22222 = CreateWorkspace([0], [0])
    group.update_workspaces([22222], counts_workspace_22222, asymmetry_workspace_22222, True)
    group.show_rebin([22222], 'counts_name_22222_rebin', 'asymmetry_name_22222_rebin')
    counts_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_33333 = CreateWorkspace([0], [0])
    group.update_workspaces([33333], counts_workspace_33333, asymmetry_workspace_33333, True)
    group.show_rebin([33333], 'counts_name_33333_rebin', 'asymmetry_name_33333_rebin')

    return group
