# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import get_run_number_from_workspace_name
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import make_group


def check_not_in_group(groups, name):
    for group in groups:
        if name in group.getNames():
            return False
    return True


def safe_to_add_to_group(name, instrument, groups, extension):
    if name.isGroup():
        return False
    return instrument in name.name() and extension in name.name() and check_not_in_group(groups, name.name())


def add_list_to_group(ws_names, group):
    for ws_name in ws_names:
        group.add(ws_name)


def add_to_group(instrument, extension):
    str_names = AnalysisDataService.getObjectNames()
    # only group things for the current instrument
    ws_list =  AnalysisDataService.retrieveWorkspaces(str_names)

    #just the groups
    groups = [name for name in ws_list if name.isGroup()]

    # just the workspaces
    def string_name(name):
        if isinstance(name, str):
            return name
        return name.name()

    names = [string_name(name) for name in ws_list if safe_to_add_to_group(name, instrument, groups, extension)]
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
