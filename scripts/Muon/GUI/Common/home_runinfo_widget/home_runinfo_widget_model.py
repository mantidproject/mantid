# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.contexts.muon_context import MuonContext

millions_counts_conversion = 1. / 1e6


class HomeRunInfoWidgetModel(object):
    def __init__(self, context=MuonContext()):
        self._data = context.data_context

    def get_run_number(self):
        return str(self._data.current_run[0]) if self._data.current_run else ''

    def get_instrument_name(self):
        inst = self._data.current_workspace.getInstrument()
        return inst.getName()

    def get_log_value(self, log_name):
        log = self._data.get_sample_log(log_name)
        if log:
            return log.value
        else:
            return "Log not found"

    def get_total_counts(self):
        workspace = self._data.current_workspace
        total = 0
        for i in range(workspace.getNumberHistograms()):
            total += sum(workspace.dataY(i))

        return total

    def get_counts_in_MeV(self, counts):
        return counts * millions_counts_conversion

    def get_counts_per_good_frame(self, counts):
        good_frames = self.get_log_value("goodfrm")

        if good_frames != 'Log not found':
            return round(counts / float(good_frames), 3)
        else:
            return 'Good frames not defined'

    def get_counts_per_good_frame_per_detector(self, counts):
        good_frames = self.get_log_value("goodfrm")

        if good_frames != 'Log not found':
            return round(counts / float(good_frames) / float(self._data.num_detectors), 3)
        else:
            return 'Good frames not defined'

    def get_average_temperature(self):
        # TODO : This implementation does not match the one in the C++ code
        # as the C++ filters the time series logs based on the start and end times.
        # TimeSeriesProperty.cpp line 934
        temps = self._data.get_sample_log("Temp_Sample")
        try:
            temps = self._data.current_workspace.getRun().getProperty("Temp_Sample")
        except Exception:
            return "Log not found"
        if temps:
            return round(temps.timeAverageValue(), 5)
        else:
            return "Log not found"

    def get_workspace_comment(self):
        ws = self._data.current_workspace
        return ws.getComment()
