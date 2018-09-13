from __future__ import (absolute_import, division, print_function)

# import mantid.simpleapi as mantid

from Muon.GUI.Common.muon_context import MuonContext


class HomeRunInfoWidgetModel(object):

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data

    def get_run_number(self):
        run = self._data.get_sample_log("run_number")
        print("Workspace methods : ", dir(self._data.loaded_data["OutputWorkspace"].value))
        print(self._data.loaded_data["OutputWorkspace"].value.getSampleDetails().keys())
        return run.value
        # self._data["OutputWorkspace"].getSampleData().getLogData("run")

    def get_instrument_name(self):
        inst = self._data.loaded_data["OutputWorkspace"].value.getInstrument()
        # print(dir(inst))
        return inst.getName()

    def get_log_value(self, log_name):
        log = self._data.get_sample_log(log_name)
        return log.value

    def get_counts_in_MeV(self):
        workspace = self._data.loaded_data["OutputWorkspace"].value
        total = 0
        for i in range(workspace.getNumberHistograms()):
            total += sum(workspace.dataY(i))
        return total / 1000000

    def get_average_temperature(self):
        # TODO : This implementation does not match the one in the C++ code
        # as the C++ filters the time series logs based on the start and end times.
        # MuonAnalysis.cpp
        # TimeSeriesProperty.cpp line 934
        temps = self._data.get_sample_log("Temp_Sample")
        temps = self._data.loaded_data["OutputWorkspace"].value.getRun().getProperty("Temp_Sample")
        # print(temps.timeAverageValue())
        # print(temps.times)
        print("dir of temps : ", dir(temps))
        return temps.timeAverageValue()

    def get_workspace_comment(self):
        ws = self._data.loaded_data["OutputWorkspace"].value
        return ws.getComment()
