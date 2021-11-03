# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import get_run_number_from_workspace_name
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import make_group


def check_not_in_group(groups, ws):
    for group in groups:
        if ws in group.getNames():
            return False
    return True


def safe_to_add_to_group(ws, instrument, groups, extension):
    if ws.isGroup():
        return False
    return instrument in ws.name() and extension in ws.name() and check_not_in_group(groups, ws.name())


def add_list_to_group(ws_names, group):
    for ws_name in ws_names:
        group.add(ws_name)


def add_to_group(instrument, extension):
    str_names = AnalysisDataService.getObjectNames()
    # only group things for the current instrument
    ws_list =  AnalysisDataService.retrieveWorkspaces(str_names)

    #just the groups
    groups = [ws for ws in ws_list if ws.isGroup()]

    # just the workspaces
    def string_name(ws):
        if isinstance(ws, str):
            return ws
        return ws.name()

    names = [string_name(ws) for ws in ws_list if safe_to_add_to_group(ws, instrument, groups, extension)]
    # make sure we include the groups that we already have in the ADS
    group_names = {key.name():[] for key in groups}
    # put ws into groups
    for name in names:
        run = get_run_number_from_workspace_name(name, instrument)
        tmp = instrument+run
        # check the names are not already group workspaces
        if tmp in list(group_names.keys()):
            group_names[tmp] += [name]
        else:
            group_names[tmp] = [name]

    # add to the groups that already exist
    for group in groups:
        if group.name() in group_names.keys():
            add_list_to_group(group_names[group.name()], group)

    # create new groups
    for group in group_names.keys():
        if group not in [group.name() for group in groups] :
            make_group(group_names[group], group)
