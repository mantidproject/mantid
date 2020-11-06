# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import WorkspaceGroup
from mantid.simpleapi import CloneWorkspace, DeleteWorkspace


def combine_loaded_runs(model, run_list, delete_added=False):
    coAddWorkspace_name = str(run_list[0]) + "-" + str(run_list[-1])
    coAddWorkspace = WorkspaceGroup()
    finished_detectors = []
    ws_remove = []
    for run in run_list:
        run_detectors = get_detectors(model, run)
        detectors_to_check = check_all_detectors_complete(run_detectors, finished_detectors)
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
                add_detector_workspace_to_group(coAddWorkspace, new_ws, new_ws_name, detector, finished_detectors,
                                                ws_remove)
    finalise_groupworkspace(coAddWorkspace, coAddWorkspace_name, ws_remove)


def check_all_detectors_complete(run_detectors, finished_detectors):
    unused_detectors = list(set(run_detectors).difference(finished_detectors))
    if unused_detectors:
        return True
    else:
        return False


def add_detector_workspace_to_group(grpWS, newWS, newWSName, detector, finished_detectors, wsList):
    """
       Adds the Detector workspace for the combined runs to the group workspace, adds the detector
       to the list of detectors that have been completed and removes any unnecessary workspaces.
    """
    grpWS.addWorkspace(newWS.clone(OutputWorkspace=newWSName))
    finished_detectors.append(detector)
    DeleteWorkspace(newWS)
    wsList.append(newWSName)


def finalise_groupworkspace(grpWS, grpWSName, wsList):
    """
       Adds the groupworkspace with suitable name and removes any unnecessary workspaces
    """
    grpWS.clone(OutputWorkspace=grpWSName)
    for ws in wsList:
        DeleteWorkspace(ws)


def find_ws_to_use(model, run_detectors, detector, run):
    workspace_place = run_detectors.index(detector)
    ws = model._loaded_data_store.get_data(run=[run])["workspace"].getItem(workspace_place)
    return ws


def get_detectors(model, run):
    return model._data_context._run_info.get(run)._detectors
