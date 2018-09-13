from __future__ import (absolute_import, division, print_function)

# import mantid.simpleapi as mantid

from Muon.GUI.Common.muon_context import MuonContext


class MuonPreProcessParameters(object):

    def __init__(self):
        self._loaded_time_zero = 0.0
        self._time_zero = 0.0
        self._first_good_data = 0.0
        self._rebin = 5
        # TODO : How to handle dead times?
        self._dead_times = []

    @property
    def time_offset(self):
        return self._loaded_time_zero - self.time_zero


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
        self._active_run = None

    def clear_data(self):
        """When e.g. instrument changed"""
        self._data.clear()

    def get_file_time_zero(self):
        return self._data.loaded_data["TimeZero"].value



    def get_user_time_zero(self):
        return 0.0

    def set_time_zero_to_file(self):
        pass

    def modify_time_zero(self, new_time_zero):
        pass

    def get_file_first_good_data(self):
        return self._data.loaded_data["FirstGoodData"].value

    def get_user_first_good_data(self):
        return 0.0

    def set_first_good_data_all(self, first_good_data):
        """
        In the context itself :

        def apply_to_runs(runs, callable, *args, **kwargs):
            ...
            applies a function to selected runs.
        """

        pass

    def set_first_good_data(self, first_good_data):
        pass

    def set_first_good_data_to_file(self):
        pass

    def get_first_good_data(self):
        pass

    def set_dead_time_to_file(self):
        pass

    def get_dead_time_table_from_data(self):
        return self._data.loaded_data["DeadTimeTable"].value

    def load_dead_time(self):
        pass
        # TODO : Create loader class which handles loading workspaces as well as dead times
        # this way, we have all the loading algorithms in one place...
