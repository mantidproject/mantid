from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.load_utils as load_utils

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.load_utils import MuonWorkspace

from Muon.GUI.Common.muon_load_data import MuonLoadData

import Muon.GUI.Common.run_string_utils as run_string_utils

from collections import OrderedDict

from mantid.simpleapi import mtd
from mantid.kernel import ConfigServiceImpl
from mantid import api
import mantid as mantid


class MuonContext(object):

    def __init__(self):
        self._groups = OrderedDict()
        self._pairs = OrderedDict()

        self._loaded_data = MuonLoadData()
        self._current_data = self.get_result()

        print(self._current_data)

    def update_current_data(self):

        group_name_bank = ["fwd", "bwd", "top", "bottom"]
        pair_name_bank = ["long1", "long2"]

        if self._loaded_data.num_items() > 0:
            print("LOADING")
            self._current_data = self._loaded_data.params["workspace"][-1]

            # handle making groups/pairs
            grouping_table = self._current_data["DetectorGroupingTable"].value
            print("Grouping Table : ", grouping_table.toDict())
            for i, detector_list in enumerate(grouping_table.toDict()["Detectors"]):
                print("\t", detector_list)
                print("\t", run_string_utils.run_list_to_string(detector_list))
                new_group = MuonGroup(group_name=group_name_bank[i], detector_IDs=detector_list)
                # new_group.name = group_name_bank[i]
                self._groups[group_name_bank[i]] = new_group
            self.get_default_grouping("EMU")

    def is_multi_period(self):
        return isinstance(self._current_data["OutputWorkspace"], list)

    def get_sample_log(self, log_name):

        log = None
        logs = None
        try:
            logs = self.loaded_workspace.getSampleDetails()
        except Exception:
            print("oh dear")

        # print("dir logs", dir(logs))
        if logs:
            try:
                log = logs.getLogData(log_name)
            except:
                print("Cant find log")
        return log

    def get_result(self):
        # filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\multi_period_data\EMU00083015.nxs"
        filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020883.nxs"
        result, _run, _filename = load_utils.load_workspace_from_filename(filename)

        self._groups["fwd"] = MuonGroup()
        self._groups["bwd"] = MuonGroup()
        self._pairs["long"] = MuonPair()

        return result

    def get_result_2(self):
        filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\multi_period_data\EMU00083015.nxs"
        # filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020884.nxs"
        result, _run, _filename = load_utils.load_workspace_from_filename(filename)

        self._groups["fwd2"] = MuonGroup()
        self._groups["bwd2"] = MuonGroup()
        self._pairs["long2"] = MuonPair()

        return result

    @property
    def loaded_data(self):
        return self._current_data

    @property
    def loaded_workspace(self):
        if isinstance(self._current_data["OutputWorkspace"], list):
            return self._current_data["OutputWorkspace"][0].workspace
        else:
            return self._current_data["OutputWorkspace"].workspace

    def clear(self):
        self._groups = OrderedDict()
        self._pairs = OrderedDict()
        self._current_data = self.get_result_2()

    @property
    def instrument(self):
        return self.loaded_workspace.getInstrument().getName()

    @property
    def run(self):
        return self.get_sample_log("run_number").value

    @property
    def period_string(self):
        # Get the period string i.e. "1+2-3+4" to be used in workspace naming.
        return "1"

    def get_raw_data_workspace_name(self):
        instrument = self.instrument
        run = self.run
        return str(instrument) + str(run) + "_raw_data"

    def get_raw_data_directory(self):
        instrument = self.instrument
        run = self.run
        if isinstance(self.loaded_data["OutputWorkspace"].workspace, api.WorkspaceGroup):
            periods = "1"
            dir = "Muon Data/" + str(instrument) + str(run) + " Period " + periods + "/"
            return dir
        else:
            return "Muon Data/" + str(instrument) + str(run) + "/"

    def get_cached_data_directory(self):
        pass

    def get_group_data_directory(self):
        pass

    def get_pair_data_directory(self):
        pass

    def show_raw_data(self):
        ws = self.loaded_data["OutputWorkspace"]
        # print("SHOW RAW DATA")
        names = []
        if isinstance(ws, list):
            for i, single_ws in enumerate(ws):
                name = self.get_raw_data_workspace_name() + "_period_" + str(i)
                names += [name]
                single_ws.show(name=name)
        else:
            name = self.get_raw_data_workspace_name()
            names += [name]
            ws.show(name=name)
        # print(api.AnalysisDataServiceImpl.Instance().getObjectNames())

    def show_group_data(self, group_name):
        # TODO : complete
        name = self.get_group_data_workspace_name(group_name)
        directory = ""
        ws = self.calculate_group_data(group_name)
        self._groups[group_name].workspace = MuonWorkspace(ws)
        self._groups[group_name].workspace.show(name)

    def show_all_groups(self):
        for group_name in self._groups.keys():
            self.show_group_data(group_name)

    def show_pair_data(self, pair_name):
        # TODO : complete
        name = self.get_pair_data_workspace_name(pair_name)
        directory = ""

    def get_group_data_workspace_name(self, group_name):
        instrument = self.instrument
        run = self.run
        if self.is_multi_period():
            return str(instrument) + str(
                run) + "; Group; " + group_name + "; Counts; Periods; " + self.period_string + "; #1"
        else:
            return str(instrument) + str(run) + "; Group; " + group_name + "; Counts; #1"

    def get_pair_data_workspace_name(self, pair_name):
        instrument = self.instrument
        run = self.run
        if self.is_multi_period():
            return str(instrument) + str(
                run) + "; Group; " + pair_name + "; Periods; " + self.period_string + "; #1"
        else:
            return str(instrument) + str(run) + "; Group; " + pair_name + "; #1"

    def calculate_all_groups(self):
        for group_name in self._groups.keys():
            self.calculate_group_data(group_name)

    def calculate_group_data(self, group_name):
        ws = self.loaded_workspace

        api.AnalysisDataServiceImpl.Instance().add("input", ws)

        alg = mantid.AlgorithmManager.create("MuonPreProcess")
        alg.initialize()
        alg.setAlwaysStoreInADS(False)
        alg.setProperty("OutputWorkspace", "__notUsed")
        alg.setProperty("InputWorkspace", "input")
        # alg.setProperties(property_dictionary)
        alg.execute()

        api.AnalysisDataServiceImpl.Instance().remove("input")

        wsOut = alg.getProperty("OutputWorkspace").value
        api.AnalysisDataServiceImpl.Instance().add("wsOut", wsOut)

        alg = mantid.AlgorithmManager.create("MuonGroupingCounts")
        alg.initialize()
        alg.setAlwaysStoreInADS(False)
        alg.setProperty("OutputWorkspace", "__notUsed")
        alg.setProperty("InputWorkspace", "wsOut")
        alg.setProperty("GroupName", "group")
        alg.setProperty("Grouping", "1,2,3,4,5")
        alg.setProperty("SummedPeriods", "1")
        alg.setProperty("SubtractedPeriods", "")
        alg.execute()

        wsOut2 = alg.getProperty("OutputWorkspace").value
        api.AnalysisDataServiceImpl.Instance().remove("wsOut")
        api.AnalysisDataServiceImpl.Instance().remove("wsOut_1")

        print("ADS : ", api.AnalysisDataServiceImpl.Instance().getObjectNames())
        return wsOut2

    def calculate_pair_data(self, pair_name):
        ws = self.loaded_workspace

        api.AnalysisDataServiceImpl.Instance().add("input", ws)

        alg = mantid.AlgorithmManager.create("MuonPreProcess")
        alg.initialize()
        alg.setAlwaysStoreInADS(False)
        alg.setProperty("OutputWorkspace", "__notUsed")
        alg.setProperty("InputWorkspace", "input")
        # alg.setProperties(property_dictionary)
        alg.execute()

        wsOut = alg.getProperty("OutputWorkspace").value
        api.AnalysisDataServiceImpl.Instance().remove("input")

        api.AnalysisDataServiceImpl.Instance().add("wsOut", wsOut)

        alg = mantid.AlgorithmManager.create("MuonPairingAsymmetry")
        alg.initialize()
        alg.setAlwaysStoreInADS(False)
        alg.setProperty("OutputWorkspace", "__notUsed")
        alg.setProperty("InputWorkspace", "wsOut")
        alg.setProperty("PairName", "pair")
        alg.setProperty("Group1", "1,2,3,4,5")
        alg.setProperty("Group2", "6,7,8,9,10")
        alg.setProperty("SummedPeriods", "1")
        alg.setProperty("SubtractedPeriods", "")
        alg.execute()

        wsOut2 = alg.getProperty("OutputWorkspace").value
        api.AnalysisDataServiceImpl.Instance().remove("wsOut")

        return wsOut2

    def get_default_grouping(self, instrument):
        parameter_name = "Default grouping file"

        grouping_file = self._current_data["OutputWorkspace"].workspace.getInstrument().getStringParameter(
            parameter_name)[0]
        instrument_directory = ConfigServiceImpl.Instance().getInstrumentDirectory()

        print("dir : ", dir(self._current_data["OutputWorkspace"].workspace))
        print("dir : ",
              self._current_data["OutputWorkspace"].workspace.getInstrument())

        print(ConfigServiceImpl.Instance().getInstrumentDirectory())

        filename = instrument_directory + grouping_file
        load_utils.load_grouping_from_XML(filename)