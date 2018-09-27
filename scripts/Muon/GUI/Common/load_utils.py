from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid
from mantid.api import WorkspaceGroup
from mantid.api import ITableWorkspace
from mantid.simpleapi import mtd
from mantid import api
from mantid.kernel import ConfigServiceImpl
import mantid.kernel as kernel
import xml.etree.ElementTree as ET
import Muon.GUI.Common.run_string_utils as run_string_utils
import Muon.GUI.Common.muon_file_utils as file_utils

from Muon.GUI.Common.muon_workspace import MuonWorkspace

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair


class LoadUtils(object):
    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MonAnalysis
    """

    def __init__(self, parent=None):
        exists, tmpWS = self.MuonAnalysisExists()
        if exists:
            self.setUp(tmpWS)
        else:
            raise RuntimeError("No data loaded. \n Please load data using Muon Analysis")

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
        default_instrument = "EMU"
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
    # alg.setProperty("Workspace", "__notUsed")
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
    return MuonWorkspace(workspace)


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
DEFAULT_OUTPUT_VALUES = [__default_workspace(),
                         None,  # api.WorkspaceFactoryImpl.Instance().createTable("TableWorkspace"),
                         api.WorkspaceFactoryImpl.Instance().createTable("TableWorkspace"),
                         0.0,
                         0.0, "Unknown direction"]


class floatPropertyWithValue(object):
    """
    Simple class to replicate the functionality of the mantid "PropertyWithValue", allowing this class
    to take its place when, for example, using a dummy load result.
    """

    def __init__(self, value):
        self._value = value

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, new_value):
        self._value = new_value


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
    loaded_data, run, _ = load_workspace_from_filename(filename)

    if is_workspace_group(loaded_data["OutputWorkspace"]):
        dead_times = loaded_data["DataDeadTimeTable"][0]
    else:
        dead_times = loaded_data["DataDeadTimeTable"]

    if dead_times is None:
        return ""
    assert isinstance(dead_times, ITableWorkspace)

    instrument = loaded_data["OutputWorkspace"].workspace.getInstrument().getName()
    name = str(instrument) + file_utils.format_run_for_file(run) + "_deadTimes"
    api.AnalysisDataService.Instance().addOrReplace(name, dead_times)

    return name


def load_workspace_from_filename(filename,
                                 input_properties=DEFAULT_INPUTS,
                                 output_properties=DEFAULT_OUTPUTS):
    try:
        alg = create_load_algorithm(filename, input_properties)
        alg.execute()
    except:
        alg = create_load_algorithm(filename.split(os.sep)[-1], input_properties)
        alg.execute()

    workspace = alg.getProperty("OutputWorkspace").value
    if is_workspace_group(workspace):
        # handle multi-period data
        load_result = _get_algorithm_properties(alg, output_properties)
        load_result["OutputWorkspace"] = [MuonWorkspace(ws) for ws in load_result["OutputWorkspace"]]
        run = get_run_from_multi_period_data(workspace)

    else:
        # single period data
        load_result = _get_algorithm_properties(alg, output_properties)
        load_result["OutputWorkspace"] = MuonWorkspace(load_result["OutputWorkspace"])
        run = int(workspace.getRunNumber())

    load_result["DataDeadTimeTable"] = load_result["DeadTimeTable"]
    load_result["DeadTimeTable"] = None

    filename = alg.getProperty("Filename").value

    return load_result, run, filename


def empty_loaded_data():
    return dict(zip(DEFAULT_OUTPUTS + ["DataDeadTimeTable"], DEFAULT_OUTPUT_VALUES + [None]))


def create_load_algorithm(filename, property_dictionary):
    alg = mantid.AlgorithmManager.create("LoadMuonNexus")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperty("Filename", filename)
    alg.setProperties(property_dictionary)
    return alg


def _get_algorithm_properties(alg, property_dict):
    return {key: alg.getProperty(key).value for key in alg.keys() if key in property_dict}


def get_table_workspace_names_from_ADS():
    """
    Return a list of names of TableWorkspace objects which are in the ADS.
    """
    names = api.AnalysisDataService.Instance().getObjectNames()
    table_names = [name for name in names if isinstance(mtd[name], ITableWorkspace)]
    return table_names


# ----------------------------------------------------------------------------------------------------------------------
# Saving/Loading grouping XML
# ----------------------------------------------------------------------------------------------------------------------


def _create_XML_subElement_for_groups(root_node, groups):
    group_nodes = []
    for group in groups:
        child = ET.SubElement(root_node, 'group', name=group.name)
        id_string = run_string_utils.run_list_to_string(group.detectors)
        ids = ET.SubElement(child, 'ids', val=id_string)
        child.extend(ids)
        group_nodes += [child]
    return group_nodes


def _create_XML_subElement_for_pairs(root_node, pairs):
    pair_nodes = []
    for pair in pairs:
        child = ET.SubElement(root_node, 'pair', name=pair.name)
        fwd_group = ET.SubElement(child, 'forward-group', val=pair.group1)
        bwd_group = ET.SubElement(child, 'backward-group', val=pair.group2)
        alpha = ET.SubElement(child, 'alpha', val=str(pair.alpha))
        child.extend(fwd_group)
        child.extend(bwd_group)
        child.extend(alpha)
        pair_nodes += [child]
    return pair_nodes


def save_grouping_to_XML(groups, pairs, filename, save=True):
    """
    Save a set of muon group and pair parameters to XML format file. Fewer checks are performed
    than with the XML loading.

    :param groups: A list of MuonGroup objects to save.
    :param pairs: A list of MuonPair objects to save.
    :param filename: The name of the XML file to save to.
    :param save: Whether to actually save the file.
    :return: the XML tree (used in testing).
    """
    # some basic checks
    if filename == "":
        raise AttributeError("File must be specified for saving to XML")
    if os.path.splitext(filename)[-1].lower() != ".xml":
        raise AttributeError("File extension must be XML")
    if sum([0 if isinstance(group, MuonGroup) else 1 for group in groups]) > 0:
        raise AttributeError("groups must be MuonGroup type")
    if sum([0 if isinstance(pair, MuonPair) else 1 for pair in pairs]) > 0:
        raise AttributeError("pairs must be MuonPair type")

    root = ET.Element("detector-grouping")

    # handle groups
    group_nodes = _create_XML_subElement_for_groups(root, groups)
    for child in group_nodes:
        root.extend(child)

    # handle pairs
    pair_nodes = _create_XML_subElement_for_pairs(root, pairs)
    for child in pair_nodes:
        root.extend(child)

    tree = ET.ElementTree(root)
    if save:
        tree.write(filename)
    return tree


def load_grouping_from_XML(filename):
    """
    Load group/pair data from an XML file (which can be produced using the save_grouping_to_XML() function

    :param filename: Full filepath to an xml file.
    :return: (groups, pairs), lists of MuonGroup, MuonPair objects respectively.
    """
    tree = ET.parse(filename)
    root = tree.getroot()

    group_names, group_ids = _get_groups_from_XML(root)
    pair_names, pair_groups, pair_alphas = _get_pairs_from_XML(root)
    groups, pairs = [], []

    for i, group_name in enumerate(group_names):
        groups += [MuonGroup(group_name=group_name,
                             detector_ids=group_ids[i])]
    for i, pair_name in enumerate(pair_names):
        pairs += [MuonPair(pair_name=pair_name,
                           group1_name=pair_groups[i][0],
                           group2_name=pair_groups[i][1],
                           alpha=pair_alphas[i])]
    return groups, pairs


def _get_groups_from_XML(root):
    names, ids = [], []
    for child in root:
        if child.tag == "group":
            names += [child.attrib['name']]
            ids += [run_string_utils.run_string_to_list(child.find('ids').attrib['val'])]
    return names, ids


def _get_pairs_from_XML(root):
    names, groups, alphas = [], [], []
    for child in root:
        if child.tag == "pair":
            names += [child.attrib['name']]
            groups += [[child.find('forward-group').attrib['val'], child.find('backward-group').attrib['val']]]
            alphas += [child.find('alpha').attrib['val']]
    return names, groups, alphas


# ----------------------------------------------------------------------------------------------------------------------
# Mantid algorithms
# ----------------------------------------------------------------------------------------------------------------------

def run_MuonPreProcess(parameter_dict):
    """
    Apply the MuonPreProcess algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
    print("Pre-process : ", {key: val for key, val in parameter_dict.items() if key != "InputWorkspace"})
    if "DeadTimeTable" in parameter_dict.keys():
        print("DTC : ", type(parameter_dict["DeadTimeTable"]), parameter_dict["DeadTimeTable"].toDict())
    alg = mantid.AlgorithmManager.create("MuonPreProcess")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_MuonGroupingCounts(parameter_dict):
    """
    Apply the MuonGroupingCounts algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
    alg = mantid.AlgorithmManager.create("MuonGroupingCounts")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_MuonPairingAsymmetry(parameter_dict):
    """
    Apply the MuonPairingAsymmetry algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
    alg = mantid.AlgorithmManager.create("MuonPairingAsymmetry")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_AppendSpectra(ws1, ws2):
    """
    Apply the AppendSpectra algorithm to two given workspaces (no checks made).
    Returns the appended workspace.
    """
    alg = mantid.AlgorithmManager.create("AppendSpectra")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperty("InputWorkspace1", ws1)
    alg.setProperty("InputWorkspace2", ws2)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def run_AlphaCalc(parameter_dict):
    """
    Apply the AlphaCalc algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated value of alpha.
    """
    alg = mantid.AlgorithmManager.create("AlphaCalc")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("Alpha").value


def run_Plus(parameter_dict):
    """
    Apply the AlphaCalc algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated value of alpha.
    """
    alg = mantid.AlgorithmManager.create("Plus")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperties(parameter_dict)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value
