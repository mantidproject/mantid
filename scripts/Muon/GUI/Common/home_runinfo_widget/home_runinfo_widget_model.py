"""
The run information box of the home tab of Muon Analysis 2.0. This file
contains the model class HomeRunInfoWidgetModel.
"""

from __future__ import (absolute_import, division, print_function)

import dateutil.parser

from Muon.GUI.Common.muon_context import MuonContext


class HomeRunInfoWidgetModel(object):
    """
    Model class for the MVP run information widget, part of the home tab
    of Muon Analysis 2.0.
    """
    # Default string if a requested log is not found
    LOG_NOT_FOUND_MESSAGE = "Log not found"

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data

    def get_run_number(self):
        """Run number for currently loaded data."""
        return str(self._data.run)

    def get_instrument_name(self):
        """Instrument name for currently loaded data."""
        inst = self._data.loaded_workspace.getInstrument()
        return inst.getName()

    def get_log_value(self, log_name):
        """
        Attempt to extract a sample log value from the loaded workspace.
        :param log_name: Name of the sample log, string.
        :return: The log value, or None
        """
        log = self._data.get_sample_log(log_name)
        if log:
            return log.value
        else:
            return None

    def get_counts_in_MeV(self):
        """Total counts (in MeV) for currently loaded data."""
        workspace = self._data.loaded_workspace
        total = 0
        for i in range(workspace.getNumberHistograms()):
            total += sum(workspace.dataY(i))
        return total / 1000000

    def get_average_temperature(self):
        """
        Get the average temperature for the currently loaded
        data.
        """
        # TODO : This implementation does not match the one in the C++ code
        # (as the C++ filters the time series logs based on the start and end times.
        # TimeSeriesProperty.cpp line 934)
        temps = self._data.get_sample_log("Temp_Sample")
        try:
            temps = self._data.loaded_workspace.getRun().getProperty("Temp_Sample")
        except Exception:
            return None
        if temps:
            return temps.timeAverageValue()
        else:
            return None

    def get_start_time(self):
        """
        Get ISO 8601 date time string from sample logs and format as
        e.g. 01-Oct-2018 14:23:30
        """
        run_start = self.get_log_value("run_start")
        if run_start:
            datetime = dateutil.parser.parse(run_start)
            return datetime.strftime("%Y-%b-%d %H:%M:%S%z")
        else:
            return self.LOG_NOT_FOUND_MESSAGE

    def get_end_time(self):
        """
        Get ISO 8601 date time string from sample logs and format as
        e.g. 01-Oct-2018 14:23:30
        """
        run_start = self.get_log_value("run_end")
        if run_start:
            datetime = dateutil.parser.parse(run_start)
            return datetime.strftime("%Y-%b-%d %H:%M:%S%z")
        else:
            return self.LOG_NOT_FOUND_MESSAGE

    def get_workspace_comment(self):
        """
        Get the run comment for the currently loaded workspace.
        """
        workspace = self._data.loaded_workspace
        return workspace.getComment()
