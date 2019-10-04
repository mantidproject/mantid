# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid
from mantid.api import WorkspaceGroup, AnalysisDataService
from mantid.api import ITableWorkspace
from mantid.simpleapi import mtd, UnGroupWorkspace, Plus, DeleteWorkspace, CloneWorkspace
from mantid import api
from mantid.kernel import ConfigServiceImpl
import Muon.GUI.Common.utilities.muon_file_utils as file_utils
from Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


class LoadUtils(object):
    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MuonAnalysis
    """

    def __init__(self, parent=None):
        exists, tmpWS = self.MuonAnalysisExists()
        if exists:
            self.setUp(tmpWS)
        else:
            raise RuntimeError("No data loaded. \n Please load data using Muon Analysis")

    @property
    def version(self):
        return 1

    def setUp(self, tmpWS):
        # get everything from the ADS
        self.options = mantid.AnalysisDataService.getObjectNames()
        self.options = [item.replace(" ", "") for item in self.options]
        self.N_points = len(tmpWS.readX(0))
        self.instrument = tmpWS.getInstrument().getName()

        self.runName = self.instrument + str(tmpWS.getRunNumber()).zfill(8)

    # get methods
    def getNPoints(self):
        return self.N_points

    def getCurrentWS(self):
        return self.runName, self.options

    def getRunName(self):
        return self.runName

    def getInstrument(self):
        return self.instrument

    # check if data matches current
    def digit(self, x):
        return int(filter(str.isdigit, x) or 0)

    def hasDataChanged(self):
        exists, ws = self.MuonAnalysisExists()
        if exists:
            current = ws.getInstrument().getName() + str(ws.getRunNumber()).zfill(8)
            if self.runName != current:
                mantid.logger.error("Active workspace has changed. Reloading the data")
                self.setUp(ws)
                return True
        return False

    # check if muon analysis exists
    def MuonAnalysisExists(self):
        # if period data look for the first period
        if mantid.AnalysisDataService.doesExist("MuonAnalysis_1"):
            tmpWS = mantid.AnalysisDataService.retrieve("MuonAnalysis_1")
            return True, tmpWS
            # if its not period data
        elif mantid.AnalysisDataService.doesExist("MuonAnalysis"):
            tmpWS = mantid.AnalysisDataService.retrieve("MuonAnalysis")
            return True, tmpWS
        else:
            return False, None

    # Get the groups/pairs for active WS
    # ignore raw files
    def getWorkspaceNames(self):
        # gets all WS in the ADS
        runName, options = self.getCurrentWS()
        final_options = []
        # only keep the relevant WS (same run as Muon Analysis)
        for pick in options:
            if ";" in pick and "Raw" not in pick and runName in pick:
                final_options.append(pick)
        return final_options

    # Get the groups/pairs for active WS
    def getGroupedWorkspaceNames(self):
        # gets all WS in the ADS
        runName, options = self.getCurrentWS()
        final_options = []
        # only keep the relevant WS (same run as Muon Analysis)
        for pick in options:
            if "MuonAnalysisGrouped_" in pick and ";" not in pick:
                final_options.append(pick)
        return final_options


def get_default_instrument():
    default_instrument = ConfigServiceImpl.Instance().getInstrument().name()
    if default_instrument not in file_utils.allowed_instruments:
        default_instrument = 'MUSR'
    return default_instrument


def run_LoadInstrument(parameter_dict):
    """
    Apply the LoadInstrument algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated value of alpha.
    """
    alg = mantid.AlgorithmManager.create("LoadInstrument")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("Workspace").value


def __default_workspace():
    default_instrument = get_default_instrument()
    workspace = api.WorkspaceFactoryImpl.Instance().create("Workspace2D", 2, 10, 10)
    workspace = run_LoadInstrument(
        {"Workspace": workspace,
         "RewriteSpectraMap": True,
         "InstrumentName": default_instrument})
    return MuonWorkspaceWrapper(workspace)


# Dictionary of (property name):(property value) pairs to put into Load algorithm
# NOT including "OutputWorkspace" and "Filename"
DEFAULT_INPUTS = {
    "DeadTimeTable": "__notUsed",
    "DetectorGroupingTable": "__notUsed"}
# List of property names to be extracted from the result of the Load algorithm
DEFAULT_OUTPUTS = ["OutputWorkspace",
                   "DeadTimeTable",
                   "DetectorGroupingTable",
                   "TimeZero",
                   "FirstGoodData",
                   "MainFieldDirection"]
# List of default values for the DEFAULT_OUTPUTS list
DEFAULT_OUTPUT_VALUES = [[__default_workspace()],
                         None,  # api.WorkspaceFactoryImpl.Instance().createTable("TableWorkspace"),
                         api.WorkspaceFactoryImpl.Instance().createTable("TableWorkspace"),
                         0.0,
                         0.0, "Unknown direction"]


def is_workspace_group(workspace):
    return isinstance(workspace, WorkspaceGroup)


def get_run_from_multi_period_data(workspace_list):
    """Checks if multi-period data has a single consistent
    run number and returns it, otherwise raises ValueError."""
    runs = [ws.getRunNumber() for ws in workspace_list]
    unique_runs = list(set(runs))
    if len(unique_runs) != 1:
        raise ValueError("Multi-period data contains >1 unique run number.")
    else:
        return unique_runs[0]


def load_dead_time_from_filename(filename):
    """
    From a neXus file, load the dead time ITableWorkspace from it and add to the ADS
    with a name <Instrument><Run>_deadTimes , e.g. EMU0001234_deadTimes.

    :param filename: The full path to the .nxs file.
    :return: The name of the workspace in the ADS.
    """
    loaded_data, run, _, __ = load_workspace_from_filename(filename)

    if is_workspace_group(loaded_data["OutputWorkspace"]):
        dead_times = loaded_data["DataDeadTimeTable"][0]
    else:
        dead_times = loaded_data["DataDeadTimeTable"]

    if dead_times is None:
        return ""

    return dead_times


def load_workspace_from_filename(filename,
                                 input_properties=DEFAULT_INPUTS,
                                 output_properties=DEFAULT_OUTPUTS):
    try:
        alg, psi_data = create_load_algorithm(filename, input_properties)
        alg.execute()
    except:
        alg, psi_data = create_load_algorithm(filename.split(os.sep)[-1], input_properties)
        alg.execute()

    workspace = AnalysisDataService.retrieve(alg.getProperty("OutputWorkspace").valueAsStr)
    if is_workspace_group(workspace):
        # handle multi-period data
        load_result = _get_algorithm_properties(alg, output_properties)
        load_result["OutputWorkspace"] = [MuonWorkspaceWrapper(ws) for ws in workspace.getNames()]
        run = get_run_from_multi_period_data(workspace)
        if not psi_data:
            load_result["DataDeadTimeTable"] = AnalysisDataService.retrieve(load_result["DeadTimeTable"]).getNames()[0]
            for index, deadtime_table in enumerate(AnalysisDataService.retrieve(load_result["DeadTimeTable"]).getNames()):
                if index == 0:
                    load_result["DataDeadTimeTable"] = deadtime_table
                else:
                    DeleteWorkspace(Workspace=deadtime_table)

            load_result["FirstGoodData"] = round(load_result["FirstGoodData"] - load_result['TimeZero'], 2)
            UnGroupWorkspace(load_result["DeadTimeTable"])
            load_result["DeadTimeTable"] = None
            UnGroupWorkspace(workspace.name())
        else:
            load_result["DataDeadTimeTable"] = None
            load_result["FirstGoodData"] = round(load_result["FirstGoodData"], 2)
    else:
        # single period data
        load_result = _get_algorithm_properties(alg, output_properties)
        load_result["OutputWorkspace"] = [MuonWorkspaceWrapper(load_result["OutputWorkspace"])]
        run = int(workspace.getRunNumber())
        if not psi_data:
            load_result["DataDeadTimeTable"] = load_result["DeadTimeTable"]
            load_result["DeadTimeTable"] = None
            load_result["FirstGoodData"] = round(load_result["FirstGoodData"] - load_result['TimeZero'], 2)
        else:
            load_result["DataDeadTimeTable"] = None
            load_result["FirstGoodData"] = round(load_result["FirstGoodData"], 2)

    return load_result, run, filename, psi_data


def empty_loaded_data():
    return dict(zip(DEFAULT_OUTPUTS + ["DataDeadTimeTable"], DEFAULT_OUTPUT_VALUES + [None]))


def create_load_algorithm(filename, property_dictionary):
    # Assume if .bin it is a PSI file
    psi_data = False
    output_filename = os.path.basename(filename)
    if ".bin" in filename:
        alg = mantid.AlgorithmManager.create("LoadPSIMuonBin")
        psi_data = True
    else:
        alg = mantid.AlgorithmManager.create("LoadMuonNexus")
        alg.setProperties(property_dictionary)
        alg.setProperty("DeadTimeTable", output_filename + '_deadtime_table')

    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setProperty("OutputWorkspace", output_filename)
    alg.setProperty("Filename", output_filename)
    return alg, psi_data


def _get_algorithm_properties(alg, property_dict):
    def _get_non_None_value(alg, key):
        return alg.getProperty(key).value if alg.getProperty(key).value is not None else alg.getProperty(key).valueAsStr
    return {key: _get_non_None_value(alg, key) for key in alg.keys() if key in property_dict}


def get_table_workspace_names_from_ADS():
    """
    Return a list of names of TableWorkspace objects which are in the ADS.
    """
    names = api.AnalysisDataService.Instance().getObjectNames()
    table_names = [name for name in names if isinstance(mtd[name], ITableWorkspace)]
    return table_names


def combine_loaded_runs(model, run_list, delete_added=False):
    return_ws = model._loaded_data_store.get_data(run=[run_list[0]])["workspace"]
    running_total = []

    for index, workspace in enumerate(return_ws["OutputWorkspace"]):
        running_total_item = workspace.workspace.name() + 'CoAdd'
        CloneWorkspace(InputWorkspace=workspace.workspace.name(), OutputWorkspace=running_total_item)
        for run in run_list[1:]:
            ws = model._loaded_data_store.get_data(run=[run])["workspace"]["OutputWorkspace"][index].workspace
            Plus(LHSWorkspace=running_total_item, RHSWorkspace=ws, AllowDifferentNumberSpectra=False, OutputWorkspace=running_total_item)

        running_total.append(running_total_item)

    return_ws_actual = {key: return_ws[key] for key in ['MainFieldDirection', 'TimeZero', 'FirstGoodData',
                                                        'DeadTimeTable', 'DetectorGroupingTable']}
    return_ws_actual["OutputWorkspace"] = [MuonWorkspaceWrapper(running_total_period) for running_total_period in
                                           running_total]
    return_ws_actual['DataDeadTimeTable'] = CloneWorkspace(InputWorkspace=return_ws['DataDeadTimeTable'],
                                                           OutputWorkspace=return_ws['DataDeadTimeTable'] + 'CoAdd').name()
    model._loaded_data_store.remove_data(run=flatten_run_list(run_list), instrument=model._data_context.instrument)
    model._loaded_data_store.add_data(run=flatten_run_list(run_list), workspace=return_ws_actual,
                                      filename="Co-added", instrument=model._data_context.instrument)


def flatten_run_list(run_list):
    """
    run list might be [1,2,[3,4]] where the [3,4] are co-added
    """
    new_list = []
    for run_item in run_list:
        if isinstance(run_item, int):
            new_list += [run_item]
        elif isinstance(run_item, list):
            for run in run_item:
                new_list += [run]
    return new_list


def exception_message_for_failed_files(failed_file_list):
    message = "Could not load the following files : \n "
    for failure in failed_file_list:
        message += '{} ; {}'.format(os.path.split(failure[0])[-1], failure[1])
    return message
