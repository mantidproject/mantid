# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService
import mantid.simpleapi as mantid


def remove_ws(name):
    AnalysisDataService.remove(name)


def remove_ws_if_present(name):
    if AnalysisDataService.doesExist(name):
        AnalysisDataService.remove(name)


def retrieve_ws(name):
    return AnalysisDataService.retrieve(name)


def check_if_workspace_exist(name):
    return AnalysisDataService.doesExist(name)


def add_ws_to_ads(name, workspace):
    AnalysisDataService.addOrReplace(name, workspace)


# these are algs, but are related to the ADS
def delete_ws(name):
    alg = mantid.AlgorithmManager.create("DeleteWorkspace")
    alg.initialize()
    alg.setProperty("Workspace", name)
    alg.execute()


def make_group(ws_list, group_name):
    alg = mantid.AlgorithmManager.create("GroupWorkspaces")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("InputWorkspaces", ws_list)
    alg.setProperty("OutputWorkspace", group_name)
    alg.execute()
