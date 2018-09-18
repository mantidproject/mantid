from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.load_utils as load_utils

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.muon_file_utils import format_run_for_file

from collections import OrderedDict

from mantid.kernel import ConfigServiceImpl
import mantid as mantid


class MuonContext(object):
    base_directory = "Muon Data"

    def __init__(self):
        self._groups = OrderedDict()
        self._pairs = OrderedDict()

        self._loaded_data = MuonLoadData()
        self._current_data = load_utils.empty_loaded_data()  # self.get_result(False)

    @property
    def current_data(self):
        return self._current_data

    @property
    def instrument(self):
        return self.loaded_workspace.getInstrument().getName()

    @property
    def run(self):
        run_log = self.get_sample_log("run_number")
        if run_log:
            return run_log.value
        else:
            return 0

    @property
    def group_names(self):
        return self._groups.keys()

    @property
    def pair_names(self):
        return self._pairs.keys()

    @property
    def groups(self):
        return self._groups

    @property
    def pairs(self):
        return self._pairs

    def add_group(self, group):
        assert isinstance(group, MuonGroup)
        self._groups[group.name] = group

    def add_pair(self, pair):
        assert isinstance(pair, MuonPair)
        self._pairs[pair.name] = pair

    def update_current_data(self):
        # Update the current data; resetting the groups and pairs to their default values

        if self._loaded_data.num_items() > 0:
            self._current_data = self._loaded_data.params["workspace"][-1]

            groups, pairs = self.get_default_grouping("EMU")

            self.clear_groups()
            for group in groups:
                self.add_group(group)

            self.clear_pairs()
            for pair in pairs:
                self.add_pair(pair)
        else:
            self._current_data = load_utils.empty_loaded_data()

    def is_multi_period(self):
        return isinstance(self.current_data["OutputWorkspace"], list)

    def get_sample_logs(self):
        logs = None
        try:
            logs = self.loaded_workspace.getSampleDetails()
        except Exception:
            print("Cannot find sample logs")
        return logs

    def get_sample_log(self, log_name):
        logs = self.get_sample_logs()
        try:
            log = logs.getLogData(log_name)
        except Exception:
            print("Cannot find log : ", log_name)
            log = None
        return log

    @property
    def loaded_data(self):
        return self._current_data

    @property
    def loaded_workspace(self):
        if self.is_multi_period():
            # return the first workspace in the group
            return self.current_data["OutputWorkspace"][0].workspace
        else:
            return self.current_data["OutputWorkspace"].workspace

    def clear_groups(self):
        self._groups = OrderedDict()

    def clear_pairs(self):
        self._pairs = OrderedDict()

    def clear(self):
        self.clear_groups()
        self.clear_pairs()
        self._current_data = load_utils.empty_loaded_data()

    @property
    def period_string(self):
        # Get the period string i.e. "1+2-3+4" to be used in workspace naming.
        return "1"

    # ------------------------------------------------------------------------------------------------------------------
    # Workspace naming
    # ------------------------------------------------------------------------------------------------------------------

    def _base_run_name(self):
        """ e.g. EMU0001234 """
        return str(self.instrument) + format_run_for_file(int(self.run))

    def get_raw_data_workspace_name(self):
        return self._base_run_name() + "_raw_data"

    def get_group_data_workspace_name(self, group_name):
        if self.is_multi_period():
            return self._base_run_name() + "; Group; " + group_name + \
                   "; Counts; Periods; " + self.period_string + "; #1"
        else:
            return self._base_run_name() + "; Group; " + group_name + "; Counts; #1"

    def get_pair_data_workspace_name(self, pair_name):
        if self.is_multi_period():
            return self._base_run_name() + "; Pair Asym; " + pair_name + "; Periods; " + self.period_string + "; #1"
        else:
            return self._base_run_name() + "; Pair Asym; " + pair_name + "; #1"

    def get_base_data_directory(self):
        if self.is_multi_period():
            return self.base_directory + "/" + self._base_run_name() + " Period " + self.period_string + "/"
        else:
            return self.base_directory + "/" + self._base_run_name() + "/"

    def get_raw_data_directory(self):
        if self.is_multi_period():
            return self._base_run_name() + " Period " + self.period_string + "; Raw Data/"
        else:
            return self._base_run_name() + " Raw Data/"

    def get_cached_data_directory(self):
        if self.is_multi_period():
            return self._base_run_name() + " Period " + self.period_string + "; Cached/"
        else:
            return self._base_run_name() + " Cached/"

    def get_group_data_directory(self):
        if self.is_multi_period():
            return self._base_run_name() + " Period " + self.period_string + "; Groups/"
        else:
            return self._base_run_name() + " Groups/"

    def get_pair_data_directory(self):
        if self.is_multi_period():
            return self._base_run_name() + " Period " + self.period_string + "; Pairs/"
        else:
            return self._base_run_name() + " Pairs/"

    # ------------------------------------------------------------------------------------------------------------------
    # Showing workspaces in the ADS
    # ------------------------------------------------------------------------------------------------------------------

    def _show_single_workspace(self, workspace, name):
        workspace.hide()
        workspace.show(name=name)

    def show_raw_data(self):
        workspace = self.current_data["OutputWorkspace"]
        directory = self.get_base_data_directory() + self.get_raw_data_directory()

        if isinstance(workspace, list):
            # Multi-period data
            for i, single_ws in enumerate(workspace):
                name = directory + self.get_raw_data_workspace_name() + "_period_" + str(i)
                self._show_single_workspace(single_ws, name)
        else:
            # Single period data
            name = directory + self.get_raw_data_workspace_name()
            self._show_single_workspace(workspace, name)

    def show_all_groups(self):
        for group_name in self._groups.keys():
            self.show_group_data(group_name)

    def show_group_data(self, group_name):
        name = self.get_group_data_workspace_name(group_name)
        directory = self.get_base_data_directory() + self.get_group_data_directory()
        workspace = self.calculate_group_data(group_name)

        self._groups[group_name].workspace = load_utils.MuonWorkspace(workspace)
        self._groups[group_name].workspace.show(directory + name)

    def show_all_pairs(self):
        for pair_name in self._pairs.keys():
            self.show_pair_data(pair_name)

    def show_pair_data(self, pair_name):
        name = self.get_pair_data_workspace_name(pair_name)
        directory = self.get_base_data_directory() + self.get_pair_data_directory()
        workspace = self.calculate_pair_data(pair_name)

        self._pairs[pair_name].workspace = load_utils.MuonWorkspace(workspace)
        self._pairs[pair_name].workspace.show(directory + name)

    def calculate_all_groups(self):
        for group_name in self._groups.keys():
            self.calculate_group_data(group_name)

    def _get_pre_processing_params(self):
        pre_process_params = {}
        try:
            time_min = self.loaded_data["TimeMin"]
            pre_process_params["TimeMin"] = time_min
        except KeyError:
            pass

        try:
            rebin_args = self.loaded_data["Rebin"]
            pre_process_params["RebinArgs"] = rebin_args
        except KeyError:
            pass

        try:
            time_offset = self.loaded_data["TimeOffset"]
            pre_process_params["TimeOffset"] = time_offset
        except KeyError:
            pass

        try:
            dead_time_table = self.loaded_data["PreProcessDeadTimeTable"]
            pre_process_params["DeadTimeTable"] = dead_time_table
        except KeyError:
            pass

        return pre_process_params

    def _run_pre_processing(self):
        params = self._get_pre_processing_params()
        params["InputWorkspace"] = self.loaded_workspace
        processed_data = load_utils.run_MuonPreProcess(params)
        return processed_data

    def _get_MuonGroupingCounts_parameters(self, group_name):
        params = {}
        try:
            summed_periods = self.loaded_data["SummedPeriods"]
            params["SummedPeriods"] = str(summed_periods)
        except KeyError:
            params["SummedPeriods"] = "1"

        try:
            subtracted_periods = self.loaded_data["SubtractedPeriods"]
            params["SubtractedPeriods"] = str(subtracted_periods)
        except KeyError:
            params["SubtractedPeriods"] = ""

        group = self._groups.get(group_name, None)
        if group:
            params["GroupName"] = group_name
            params["Grouping"] = ",".join([str(i) for i in group.detectors])

        return params

    def calculate_group_data(self, group_name):
        processed_data = self._run_pre_processing()

        params = self._get_MuonGroupingCounts_parameters(group_name)
        params["InputWorkspace"] = processed_data
        group_data = load_utils.run_MuonGroupingCounts(params)

        return group_data

    def calculate_pair_data(self, pair_name):
        processed_data = self._run_pre_processing()

        alg = mantid.AlgorithmManager.create("MuonPairingAsymmetry")
        alg.initialize()
        alg.setAlwaysStoreInADS(False)
        alg.setProperty("OutputWorkspace", "__notUsed")
        alg.setProperty("InputWorkspace", processed_data)
        alg.setProperty("SpecifyGroupsManually", True)
        alg.setProperty("PairName", pair_name)
        alg.setProperty("Group1", "1,2,3,4,5")
        alg.setProperty("Group2", "6,7,8,9,10")
        alg.setProperty("SummedPeriods", "1")
        alg.setProperty("SubtractedPeriods", "")
        alg.execute()

        pair_data = alg.getProperty("OutputWorkspace").value

        return pair_data

    def get_default_grouping(self, _instrument):
        parameter_name = "Default grouping file"
        workspace = self.loaded_workspace
        grouping_file = workspace.getInstrument().getStringParameter(parameter_name)[0]
        instrument_directory = ConfigServiceImpl.Instance().getInstrumentDirectory()
        filename = instrument_directory + grouping_file
        new_groups, new_pairs = load_utils.load_grouping_from_XML(filename)
        return new_groups, new_pairs

    def construct_empty_group(self, _group_index):
        group_index = 0
        new_group_name = "group_" + str(group_index)
        while new_group_name in self.group_names:
            group_index += 1
            new_group_name = "group_" + str(group_index)
        return MuonGroup(group_name=new_group_name, detector_IDs=[1])

    def construct_empty_pair(self, _pair_index):
        pair_index = 0
        new_pair_name = "pair_" + str(pair_index)
        while new_pair_name in self.pair_names:
            pair_index += 1
            new_pair_name = "pair_" + str(pair_index)
        if len(self.group_names) == 1:
            group1 = self.group_names[0]
            group2 = self.group_names[0]
        elif len(self.group_names) >= 2:
            group1 = self.group_names[0]
            group2 = self.group_names[1]
        else:
            group1 = None
            group2 = None
        return MuonPair(pair_name=new_pair_name,
                        group1_name=group1, group2_name=group2, alpha=1.0)

    # ----------------------
    # DEPRECATED
    # ----------------------

    def get_result(self, groups=True):
        # filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\multi_period_data\EMU00083015.nxs"
        filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020883.nxs"
        result, _run, _filename = load_utils.load_workspace_from_filename(filename)

        if groups:
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
