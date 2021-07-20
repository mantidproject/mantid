# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService


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
