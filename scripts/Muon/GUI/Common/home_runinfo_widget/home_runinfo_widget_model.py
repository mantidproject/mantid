from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_context import MuonContext


class HomeRunInfoWidgetModel(object):

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data

    def get_run_number(self):
        return str(self._data.run)

    def get_instrument_name(self):
        inst = self._data.loaded_workspace.getInstrument()
        return inst.getName()

    def get_log_value(self, log_name):
        log = self._data.get_sample_log(log_name)
        if log:
            return log.value
        else:
            return "Log not found"

    def get_counts_in_MeV(self):
        workspace = self._data.loaded_workspace
        total = 0
        for i in range(workspace.getNumberHistograms()):
            total += sum(workspace.dataY(i))
        return total / 1000000

    def get_average_temperature(self):
        # TODO : This implementation does not match the one in the C++ code
        # as the C++ filters the time series logs based on the start and end times.
        # TimeSeriesProperty.cpp line 934
        temps = self._data.get_sample_log("Temp_Sample")
        try:
            temps = self._data.loaded_workspace.getRun().getProperty("Temp_Sample")
        except Exception:
            return "Log not found"
        if temps:
            return temps.timeAverageValue()
        else:
            return "Log not found"

    def get_workspace_comment(self):
        ws = self._data.loaded_workspace
        return ws.getComment()
