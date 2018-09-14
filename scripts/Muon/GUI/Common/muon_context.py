from __future__ import (absolute_import, division, print_function)

import Muon.GUI.Common.load_utils as load_utils

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair

from collections import OrderedDict

from mantid.simpleapi import mtd
from mantid import api


class MuonContext(object):

    def __init__(self):
        self._groups = OrderedDict()
        self._pairs = OrderedDict()

        self._loaded_data = self.get_result()

        print(self._loaded_data)

    def is_multi_period(self):
        return isinstance(self._loaded_data["OutputWorkspace"], list)

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
        result, _run = load_utils.load_workspace_from_filename(filename)

        self._groups["fwd"] = MuonGroup()
        self._groups["bwd"] = MuonGroup()
        self._pairs["long"] = MuonPair()

        return result

    def get_result_2(self):
        filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\multi_period_data\EMU00083015.nxs"
        # filename = "C:\Users\JUBT\Dropbox\Mantid-RAL\Testing\TrainingCourseData\muon_cupper\EMU00020884.nxs"
        result, _run = load_utils.load_workspace_from_filename(filename)

        self._groups["fwd2"] = MuonGroup()
        self._groups["bwd2"] = MuonGroup()
        self._pairs["long2"] = MuonPair()

        return result

    @property
    def loaded_data(self):
        return self._loaded_data

    @property
    def loaded_workspace(self):
        if isinstance(self._loaded_data["OutputWorkspace"], list):
            return self._loaded_data["OutputWorkspace"][0].workspace
        else:
            return self._loaded_data["OutputWorkspace"].workspace

    def clear(self):
        self._groups = OrderedDict()
        self._pairs = OrderedDict()
        self._loaded_data = self.get_result_2()

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
        print("SHOW RAW DATA")
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
        print(api.AnalysisDataServiceImpl.Instance().getObjectNames())

    def show_group_data(self, group_name):
        # TODO : complete
        name =self.get_group_data_workspace_name(group_name)
        directory = ""


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

    