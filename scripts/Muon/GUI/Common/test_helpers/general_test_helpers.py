# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateWorkspace
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper
from Muon.GUI.Common.muon_base import MuonRun
from Muon.GUI.Common.muon_group import MuonGroup


def create_workspace_wrapper_stub_object(name):
    workspace = CreateWorkspace([0], [0])
    wrapped_workspace = MuonWorkspaceWrapper(workspace)
    wrapped_workspace.show(name)
    return wrapped_workspace


def create_group_populated_by_two_workspace():
    group = MuonGroup(group_name="group1")
    counts_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_22222 = CreateWorkspace([0], [0])

    group.update_counts_workspace(MuonRun([22222]), counts_workspace_22222, False)
    group.update_asymmetry_workspace(MuonRun([22222]), asymmetry_workspace_22222, asymmetry_workspace_unnorm_22222, False)

    group.show_raw([22222], 'counts_name_22222', 'asymmetry_name_22222', 'asymmetry_name_22222_unnorm')
    counts_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_33333 = CreateWorkspace([0], [0])

    group.update_counts_workspace(MuonRun([33333]), counts_workspace_33333, False)
    group.update_asymmetry_workspace(MuonRun([33333]), asymmetry_workspace_33333, asymmetry_workspace_unnorm_33333, False)

    group.show_raw([33333], 'counts_name_33333', 'asymmetry_name_33333', 'asymmetry_name_33333_unnorm')

    return group


def create_group_populated_by_two_rebinned_workspaces():
    group = MuonGroup(group_name="group1")
    counts_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_22222 = CreateWorkspace([0], [0])

    group.update_counts_workspace(MuonRun([22222]), counts_workspace_22222, True)
    group.update_asymmetry_workspace(MuonRun([22222]), asymmetry_workspace_22222, asymmetry_workspace_unnorm_22222, True)

    group.show_rebin([22222], 'counts_name_22222_rebin', 'asymmetry_name_22222_rebin', 'asymmetry_name_22222_unnorm')
    counts_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_33333 = CreateWorkspace([0], [0])

    group.update_counts_workspace(MuonRun([33333]), counts_workspace_33333, True)
    group.update_asymmetry_workspace(MuonRun([33333]), asymmetry_workspace_33333, asymmetry_workspace_unnorm_33333, True)

    group.show_rebin([33333], 'counts_name_33333_rebin', 'asymmetry_name_33333_rebin', 'asymmetry_name_33333_unnorm')

    return group


def create_group_populated_by_two_binned_and_two_unbinned_workspaces():
    group = MuonGroup(group_name="group1")
    counts_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_22222 = CreateWorkspace([0], [0])
    group.update_counts_workspace(MuonRun([22222]), counts_workspace_22222, False)
    group.update_asymmetry_workspace(MuonRun([22222]), asymmetry_workspace_22222, asymmetry_workspace_unnorm_22222, False)
    group.show_raw([22222], 'counts_name_22222', 'asymmetry_name_22222', 'asymmetry_name_22222_unnorm')
    counts_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_33333 = CreateWorkspace([0], [0])

    group.update_counts_workspace(MuonRun([33333]), counts_workspace_33333, False)
    group.update_asymmetry_workspace(MuonRun([33333]), asymmetry_workspace_33333, asymmetry_workspace_unnorm_33333, False)
    group.show_raw([33333], 'counts_name_33333', 'asymmetry_name_33333', 'asymmetry_name_33333_unnorm')

    counts_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_22222 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_22222 = CreateWorkspace([0], [0])

    group.update_counts_workspace(MuonRun([22222]), counts_workspace_22222, True)
    group.update_asymmetry_workspace(MuonRun([22222]), asymmetry_workspace_22222, asymmetry_workspace_unnorm_22222, True)
    group.show_rebin([22222], 'counts_name_22222_rebin', 'asymmetry_name_22222_rebin', 'asymmetry_name_22222_unnorm')
    counts_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_33333 = CreateWorkspace([0], [0])
    asymmetry_workspace_unnorm_33333 = CreateWorkspace([0], [0])

    group.update_counts_workspace(MuonRun([33333]), counts_workspace_33333, True)
    group.update_asymmetry_workspace(MuonRun([33333]), asymmetry_workspace_33333, asymmetry_workspace_unnorm_33333, True)
    group.show_rebin([33333], 'counts_name_33333_rebin', 'asymmetry_name_33333_rebin', 'asymmetry_name_22222_unnorm')

    return group
