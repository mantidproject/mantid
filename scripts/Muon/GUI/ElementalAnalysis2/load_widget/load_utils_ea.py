# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.simpleapi import DeleteWorkspace


def combine_loaded_runs(model, run_list):
    """
        As a result of this function there should be:-
            - a groupworkspace named after the range of runs used e.g. '1234-1237'
            - a workspace for each detector named [detector]_[groupworkspace]
                e.g. 'Detector 1_1234-1237'
    """
    co_add_workspace_name = str(run_list[0]) + "-" + str(run_list[-1])
    co_add_workspace = WorkspaceGroup()
    finished_detectors = []
    ws_remove = []
    for run in run_list:
        run_detectors = get_detectors(model, run)
        detectors_to_check = check_for_unused_detectors(run_detectors, finished_detectors)
        if detectors_to_check:
            for detector in run_detectors:
                new_ws_name = detector
                new_ws = find_ws_to_use(model, run_detectors, detector, run)
                for current_run in run_list:
                    if current_run != run:
                        all_detectors = get_detectors(model, current_run)
                        if detector in all_detectors:
                            workspace_to_add = find_ws_to_use(model, run_detectors, detector, current_run)
                            new_ws = new_ws + workspace_to_add
                add_detector_workspace_to_group(co_add_workspace, new_ws, new_ws_name, detector, finished_detectors,
                                                ws_remove)
    finalise_groupworkspace(model, co_add_workspace, co_add_workspace_name, ws_remove)


def check_for_unused_detectors(run_detectors, finished_detectors):
    return list(set(run_detectors).difference(finished_detectors))


def add_detector_workspace_to_group(grpws, new_ws, new_ws_name, detector, finished_detectors, ws_list):
    """
       Adds the Detector workspace for the combined runs to the group workspace, adds the detector
       to the list of detectors that have been completed and removes any unnecessary workspaces.
    """
    grpws.addWorkspace(new_ws.clone(OutputWorkspace=new_ws_name))
    finished_detectors.append(detector)
    DeleteWorkspace(new_ws)
    ws_list.append(new_ws_name)


def finalise_groupworkspace(model, grpws, grpws_name, ws_list):
    """
       Adds the groupworkspace with suitable name and removes any unnecessary workspaces
    """
    grpws.clone(OutputWorkspace=grpws_name)
    model._loaded_data_store.add_data(run=[grpws_name], workspace=AnalysisDataService.retrieve(grpws_name))
    model._data_context._loaded_data.add_data(run=[grpws_name], workspace=AnalysisDataService.retrieve(grpws_name))
    for ws in ws_list:
        DeleteWorkspace(ws)


def find_ws_to_use(model, run_detectors, detector, run):
    workspace_place = run_detectors.index(detector)
    ws = model._loaded_data_store.get_data(run=[run])["workspace"].getItem(workspace_place)
    return ws


def get_detectors(model, run):
    run_object = next((runObject for runObject in model._data_context._run_info if runObject._run_number == run), None)
    if run_object:
        return run_object._detectors
