# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_context import MuonContext

mev_conversion_factor = 1000000


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

    def get_total_counts(self):
        workspace = self._data.loaded_workspace
        total = 0
        for i in range(workspace.getNumberHistograms()):
            total += sum(workspace.dataY(i))

        return total

    def get_counts_in_MeV(self, counts):
        return counts / mev_conversion_factor

    def get_counts_per_good_frame(self, counts):
        good_frames = self.get_log_value("goodfrm")

        return counts/good_frames

    def get_counts_per_good_frame_per_detector(self, counts):
        return self.get_counts_per_good_frame(counts)/self._data.num_detectors()

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
