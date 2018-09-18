from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_context import MuonContext
from mantid.api import ITableWorkspace
from mantid import api


class InstrumentWidgetModel(object):
    """
    The model holds the muon context and interacts with it, only able to modify the pre-processing parts of each
    run.

    The model should not take care of processing data, it should only interact with and modify the muon context data
    so that when processing is done from elsewhere the parameters of the pre-processing are up-to-date with the
    GUI.
    """

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data

    def clear_data(self):
        """When e.g. instrument changed"""
        self._data.clear()

    def get_file_time_zero(self):
        return self._data.loaded_data["TimeZero"].value

    def get_user_time_zero(self):
        if "UserTimeZero" in self._data.loaded_data.keys():
            time_zero = self._data.loaded_data["UserTimeZero"]
        else:
            # default to loaded value
            time_zero = self._data.loaded_data["TimeZero"].value
        return time_zero

    def get_file_first_good_data(self):
        return self._data.loaded_data["FirstGoodData"].value

    def get_user_first_good_data(self):
        if "UserFirstGoodData" in self._data.loaded_data.keys():
            first_good_data = self._data.loaded_data["UserFirstGoodData"]
        else:
            # Default to loaded value
            first_good_data = self._data.loaded_data["FirstGoodData"].value
        return first_good_data

    def set_user_time_zero(self, time_zero):
        self._data.loaded_data["UserTimeZero"] = time_zero

    def set_user_first_good_data(self, first_good_data):
        self._data.loaded_data["UserFirstGoodData"] = first_good_data

    def get_dead_time_table_from_data(self):
        if self._data.is_multi_period():
            return self._data.loaded_data["DeadTimeTable"].value[0]
        else:
            return self._data.loaded_data["DeadTimeTable"].value

    def load_dead_time(self):
        pass
        # TODO : Create loader class which handles loading workspaces as well as dead times
        # this way, we have all the loading algorithms in one place...

    def add_fixed_binning(self, fixed_bin_size):
        self._data.loaded_data["Rebin"] = str(fixed_bin_size)

    def add_variable_binning(self, rebin_params):
        self._data.loaded_data["Rebin"] = str(rebin_params)

    def check_dead_time_file_selection(self, selection):
        try:
            table = api.AnalysisDataServiceImpl.Instance().retrieve(str(selection))
        except Exception:
            raise ValueError("Workspace " + str(selection) + " does not exist")
        assert isinstance(table, ITableWorkspace)
        # are column names correct?
        col = table.getColumnNames()
        if len(col) != 2:
            raise ValueError("Expected 2 columns, found ", str(max(0, len(col))))
        if col[0] != "spectrum" or col[1] != "dead-time":
            raise ValueError("Columns have incorrect names")
        rows = table.rowCount()
        if rows != self._data.loaded_workspace.getNumberHistograms():
            raise ValueError("Number of histograms do not match number of rows in dead time table")
        return True
