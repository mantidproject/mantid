from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid
from mantid.api import WorkspaceGroup
from mantid.api import ITableWorkspace
from mantid.simpleapi import mtd
from mantid import api
import mantid.kernel as kernel
import xml.etree.ElementTree as ET
import Muon.GUI.Common.run_string_utils as run_string_utils

from Muon.GUI.Common.muon_workspace import MuonWorkspace

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair


def get_loaded_time_zero(workspace):
    first_good_data = 0.0
    if isinstance(workspace, WorkspaceGroup):
        if workspace.getNumberOfEntries() == 0:
            raise IndexError("Cannot find loaded time zero : Empty WorkspaceGroup")
        if "FirstGoodData" in workspace[0].getSampleDetails().keys():
            first_good_data = workspace[0].getSampleDetails().getLogData("FirstGoodData").value
    else:
        try:
            first_good_data = workspace[0].getSampleDetails().getLogData("FirstGoodData").value
        except Exception:
            raise IndexError("Cannot find loaded time zero")
    return first_good_data


def get_first_good_data(workspace):
    first_good_data = 0.0
    if isinstance(workspace, WorkspaceGroup):
        # deal with workspace groups
        if workspace.getNumberOfEntries() == 0:
            raise IndexError("Cannot find loaded time zero : Empty WorkspaceGroup")
        if "FirstGoodData" in workspace[0].getSampleDetails().keys():
            first_good_data = workspace[0].getSampleDetails().getLogData("FirstGoodData").value
    else:
        # deal with regular workspace
        try:
            if "FirstGoodData" in workspace[0].getSampleDetails().keys():
                first_good_data = workspace.getSampleDetails().getLogData("FirstGoodData").value
            else:
                raise IndexError("Cannot find loaded time zero in Workspace logs")
        except Exception:
            raise IndexError("Cannot find loaded time zero")
    return first_good_data


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


# Dictionary of (property name):(property value) pairs to put into Load algorithm
# NOT including "OutputWorkspace" and "Filename"
DEFAULT_INPUTS = {
    "DeadTimeTable": "__notUsed",
    "DetectorGroupingTable": "__notUsed"}
# List of property names to be extracted from the result of the Load algorithm
DEFAULT_OUTPUTS = ["OutputWorkspace", "DeadTimeTable", "DetectorGroupingTable", "TimeZero", "FirstGoodData"]


class floatPropertyWithValue:

    def __init__(self, value):
        self.value = value


DEFAULT_OUTPUT_VALUES = [MuonWorkspace(api.WorkspaceFactoryImpl.Instance().create("Workspace2D", 2, 10, 10)),
                         floatPropertyWithValue(api.WorkspaceFactoryImpl.Instance().createTable("TableWorkspace")),
                         floatPropertyWithValue(api.WorkspaceFactoryImpl.Instance().createTable("TableWorkspace")),
                         floatPropertyWithValue(0.0),
                         floatPropertyWithValue(0.0)]


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
    # TODO : Implement
    pass


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
        load_result["OutputWorkspace"] = [MuonWorkspace(ws) for ws in load_result["OutputWorkspace"].value]
        run = get_run_from_multi_period_data(workspace)

    else:
        # single period data
        load_result = _get_algorithm_properties(alg, output_properties)
        load_result["OutputWorkspace"] = MuonWorkspace(load_result["OutputWorkspace"].value)
        run = int(workspace.getRunNumber())

    filename = alg.getProperty("Filename").value

    return load_result, run, filename


def empty_loaded_data():
    return dict(zip(DEFAULT_OUTPUTS, DEFAULT_OUTPUT_VALUES))


def create_load_algorithm(filename, property_dictionary):
    alg = mantid.AlgorithmManager.create("LoadMuonNexus")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperty("Filename", filename)
    alg.setProperties(property_dictionary)
    return alg


def _get_algorithm_properties(alg, property_dict):
    return {key: alg.getProperty(key) for key in alg.keys() if key in property_dict}


def get_table_workspace_names_from_ADS():
    """
    Return a list of names of TableWorkspace objects which are in the ADS.
    """
    names = api.AnalysisDataService.Instance().getObjectNames()
    table_names = [name for name in names if isinstance(mtd[name], ITableWorkspace)]
    return table_names


def _get_groups_from_XML(root):
    names = []
    ids = []
    for child in root:
        if child.tag == "group":
            names += [child.attrib['name']]
            ids += [run_string_utils.run_string_to_list(child.find('ids').attrib['val'])]
    return names, ids


def _get_pairs_from_XML(root):
    names = []
    groups = []
    alphas = []
    for child in root:
        if child.tag == "pair":
            names += [child.attrib['name']]
            groups += [[child.find('forward-group').attrib['val'], child.find('backward-group').attrib['val']]]
            alphas += [child.find('alpha').attrib['val']]
    return names, groups, alphas


def save_grouping_to_XML(groups, pairs, filename):
    root = ET.Element("detector-grouping")

    # doc = ET.SubElement(root, "group")
    # ET.SubElement(doc, "field1", name="fwd").text = "some value1"
    # ET.SubElement(doc, "field2", name="asdfasd").text = "some vlaue2"

    group_nodes = []
    for group in groups:
        child = ET.SubElement(root, 'group', name=group.name)
        id_string = run_string_utils.run_list_to_string(group.detectors)
        ids = ET.SubElement(child, 'ids', val=id_string)
        child.extend(ids)
        group_nodes += [child]

    for child in group_nodes:
        root.extend(child)

    pair_nodes = []
    for pair in pairs:
        child = ET.SubElement(root, 'pair', name=pair.name)
        fwd_group = ET.SubElement(child, 'forward-group', val=pair.group1)
        bwd_group = ET.SubElement(child, 'backward-group', val=pair.group2)
        alpha = ET.SubElement(child, 'alpha', val=str(pair.alpha))
        child.extend(fwd_group)
        child.extend(bwd_group)
        child.extend(alpha)
        # root.extend(child)
        pair_nodes += [child]

    for child in pair_nodes:
        root.extend(child)

    tree = ET.ElementTree(root)
    tree.write(filename)


def load_grouping_from_XML(filename):
    tree = ET.parse(filename)
    root = tree.getroot()
    print(tree)

    group_names, group_ids = _get_groups_from_XML(root)
    pair_names, pair_groups, pair_alphas = _get_pairs_from_XML(root)
    print(group_names, group_ids)
    print(pair_names, pair_groups, pair_alphas)

    groups = []
    pairs = []

    for i, group_name in enumerate(group_names):
        groups += [MuonGroup(group_name=group_name, detector_IDs=group_ids[i])]

    for i, pair_name in enumerate(pair_names):
        pairs += [MuonPair(pair_name=pair_name,
                           group1_name=pair_groups[i][0],
                           group2_name=pair_groups[i][1],
                           alpha=pair_alphas[i])]

    # for child in root:
    #     print(child.tag, child.attrib)
    #     for childchild in child:
    #         print(childchild.tag, childchild.attrib)

    return groups, pairs


def run_MuonPreProcess(parameter_dict):
    """
    Apply the MuonPreProcess algorithm with the properties supplied through
    the input dictionary of {proeprty_name:property_value} pairs.
    Returns the calculated workspace.
    """
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
