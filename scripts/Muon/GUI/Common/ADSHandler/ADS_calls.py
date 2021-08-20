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


# this requires a call to the ADS
def get_sample_log(ws_name, log_name):
    ws = retrieve_ws(ws_name)
    run = ws.run()
    return run.getProperty(log_name).value


def get_normalisation(name):
    return float(get_sample_log(name, "analysis_asymmetry_norm"))
