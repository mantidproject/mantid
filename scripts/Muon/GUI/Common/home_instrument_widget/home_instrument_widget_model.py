# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from decimal import Decimal, InvalidOperation

from mantid import api
from mantid.api import ITableWorkspace

from Muon.GUI.Common.contexts.muon_context import MuonContext


class InstrumentWidgetModel(object):
    """
    The model holds the muon context and interacts with it, only able to modify the pre-processing parts of each
    run.

    The model should not take care of processing data, it should only interact with and modify the muon context data
    so that when processing is done from elsewhere the parameters of the pre-processing are up-to-date with the
    GUI.
    """

    def __init__(self, context=None):
        self._data = context.data_context
        self._context = context
        self._context.gui_context['RebinType'] = 'None'

    def clear_data(self):
        """When e.g. instrument changed"""
        self._data.clear()

    def get_file_time_zero(self):
        return self._data.current_data["TimeZero"]

    def get_user_time_zero(self):
        if "TimeZero" in self._context.gui_context.keys():
            time_zero = self._context.gui_context["TimeZero"]
        else:
            # default to loaded value, keep a record of the data vaue
            self._context.gui_context["TimeZero"] = self._data.current_data["TimeZero"]
            time_zero = self._context.gui_context["TimeZero"]
        return time_zero

    def set_time_zero_from_file(self, state):
        self._context.gui_context.update_and_send_signal(TimeZeroFromFile=state)

    def set_first_good_data_source(self, state):
        self._context.gui_context.update_and_send_signal(FirstGoodDataFromFile=state)

    def set_last_good_data_source(self, state):
        self._context.gui_context.update_and_send_signal(LastGoodDataFromFile=state)

    def get_file_first_good_data(self):
        return self._data.current_data["FirstGoodData"]

    def get_user_first_good_data(self):
        if "FirstGoodData" in self._context.gui_context.keys():
            first_good_data = self._context.gui_context["FirstGoodData"]
        else:
            # Default to loaded value
            self._context.gui_context["FirstGoodData"] = self._data.current_data["FirstGoodData"]
            first_good_data = self._context.gui_context["FirstGoodData"]
        return first_good_data

    def get_last_good_data(self):
        if self._data.current_runs:
            run = self._data.current_runs[0]
            return self._context.last_good_data(run)
        else:
            return 0.0

    def set_user_time_zero(self, time_zero):
        self._context.gui_context.update_and_send_signal(TimeZero=time_zero)

    def set_user_first_good_data(self, first_good_data):
        self._context.gui_context.update_and_send_signal(FirstGoodData=first_good_data)

    def set_user_last_good_data(self, last_good_data):
        self._context.gui_context.update_and_send_signal(LastGoodData=last_good_data)

    def get_dead_time_table_from_data(self):
        return self._data.current_data["DataDeadTimeTable"]

    def get_dead_time_table(self):
        return self._data.dead_time_table

    def add_fixed_binning(self, fixed_bin_size):
        self._context.gui_context.update_and_send_signal(RebinFixed=str(fixed_bin_size))

    def add_variable_binning(self, rebin_params):
        self._context.gui_context.update_and_send_signal(RebinVariable=str(rebin_params))

    def get_variable_binning(self):
        if 'RebinVariable' in self._context.gui_context:
            return self._context.gui_context['RebinVariable']
        else:
            return ''

    def update_binning_type(self, rebin_type):
        self._context.gui_context.update_and_send_signal(RebinType=rebin_type)

    # ------------------------------------------------------------------------------------------------------------------
    # Dead Time
    # ------------------------------------------------------------------------------------------------------------------
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
        if rows != self._data.current_workspace.getNumberHistograms():
            raise ValueError("Number of histograms do not match number of rows in dead time table")
        return True

    def set_dead_time_to_none(self):
        self._context.gui_context.update_and_send_signal(DeadTimeSource='None')

    def set_dead_time_from_data(self):
        self._context.gui_context.update_and_send_signal(DeadTimeSource='FromFile')

    def set_user_dead_time_from_ADS(self, name):
        self._context.gui_context['DeadTimeSource'] = 'FromADS'
        dtc = api.AnalysisDataService.retrieve(str(name))
        self._context.gui_context.update_and_send_signal(DeadTimeTable=dtc)

    def validate_variable_rebin_string(self, variable_rebin_string):
        variable_rebin_list = variable_rebin_string.split(',')
        try:
            variable_rebin_list = [Decimal(x) for x in variable_rebin_list]
        except (ValueError, InvalidOperation):
            return (False, 'Rebin entries must be numbers')

        if len(variable_rebin_list) == 0:
            return (False, 'Rebin list must be non-empty')

        if len(variable_rebin_list) == 1:
            return (True, '')

        if len(variable_rebin_list) == 2:
            if variable_rebin_list[1] > variable_rebin_list[0]:
                return (True, '')
            else:
                return (False, 'End of range must be greater than start of range')

        while len(variable_rebin_list) >= 3:
            # We don't do any additional checking of logarithmic binning so just return true in this instance
            if variable_rebin_list[1] <= 0:
                return (True, '')

            if (variable_rebin_list[2] - variable_rebin_list[0])%variable_rebin_list[1] != 0:
                return (False, 'Step and bin boundaries must line up')

            variable_rebin_list = variable_rebin_list[2:]

        if len(variable_rebin_list) == 1:
            return (True, '')
        else:
            return (False, 'Variable rebin string must have 2 or an odd number of entires')
