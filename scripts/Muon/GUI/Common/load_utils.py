from __future__ import (absolute_import, division, print_function)


import mantid.simpleapi as mantid
from mantid.api import WorkspaceGroup
from mantid.api import ITableWorkspace
from mantid.simpleapi import mtd
from mantid import api


class MuonWorkspace(object):
    """A basic muon workspace which is either the workspace or the name of the workspace in the ADS"""

    def __init__(self, workspace):
        self._isInADS = False
        self._workspace = workspace
        self._directory = ""
        self._name = ""

    @property
    def name(self):
        return self._directory + self._name

    @name.setter
    def name(self, full_name):
        self._directory = full_name.split("/")[0]
        self._name = full_name.split("/")[-1]

    @property
    def workspace(self):
        print("GETTING WORKSPACE")
        if self._isInADS:
            print("\tis in ADS!")
            return mtd[self._name]
        else:
            print ("\tis not in ADS!")
            return self._workspace

    @workspace.setter
    def workspace(self, value):
        print("SETTING WORKSPACE")
        # TODO : add isinstance checks
        if self._isInADS:
            mtd.remove(self._name)
            self._isInADS = False
            self._name = ""
            self._directory = ""
        self._workspace = value

    def show(self, name):
        print("SHOWING WORKSPACE, NAME : ", str(name), " WORKSPACE ", type(self._workspace))
        if len(name) > 0:
            self._name = str(name)
            mtd.addOrReplace(str(self._name), self._workspace)
            if self._directory != "":
                # Add to the appropriate group
                group = self._directory.split("/")[-1]
                mtd[group].add(self._name)
            self._workspace = None
            self._isInADS = True
        else:
            print("Cannot store is ADS : name is empty")
            pass

    def hide(self):
        try:
            print("Y : ", mtd[self._name].readY(0))
            self._workspace = mtd[self._name]
            print("here")
            mtd.remove(self._name)
            self._name = ""
            self._isInADS = False
        except:
            print("Cannot remove from ADS")
            pass

    def add_directory_structure(self):

        dirs = self._directory.split("/")
        for directory in dirs:
            try:
                mtd[directory]
            except KeyError:
                group = api.WorkspaceGroup()
                mtd.addOrReplace(directory, group)

            if not isinstance(mtd[directory], api.WorkspaceGroup):
                break
            # add an else for if workspace not a group

        last_dir = ""
        for i, directory in enumerate(dirs):
            if i == 0:
                last_dir = directory
                continue
            mtd[last_dir].add(directory)
            last_dir = directory



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
    alg = create_load_algorithm(filename, input_properties)
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

    return load_result, run


def create_load_algorithm(filename, property_dictionary):
    alg = mantid.AlgorithmManager.create("LoadMuonNexus")
    alg.initialize()
    alg.setAlwaysStoreInADS(False)
    alg.setProperty("OutputWorkspace", "__notUsed")
    alg.setProperty("Filename", filename)
    alg.setProperties(property_dictionary)
    return alg


def _get_algorithm_properties(alg, property_dict):
    print(alg.keys())
    return {key: alg.getProperty(key) for key in alg.keys() if key in property_dict}


def get_table_workspace_names_from_ADS():
    names = api.AnalysisDataService.Instance().getObjectNames()
    table_names = []
    for name in names:
        if isinstance(mtd[name], ITableWorkspace):
            table_names += [name]
    return table_names



if __name__ == "__main__":
    filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\multi_period_data\EMU00083015.nxs"
    # filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020882.nxs"

    result = load_workspace_from_filename(filename)
    print(result)
