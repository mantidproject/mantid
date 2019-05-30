# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.kernel import FloatTimeSeriesProperty
import numpy as np

# Constants
DEFAULT_TABLE_NAME = 'ResultsTable'
ALLOWED_NON_TIME_SERIES_LOGS = ("run_number", "run_start", "run_end", "group",
                                "period", "sample_temp", "sample_magn_field")
# This is not a particularly robust way of ignoring this as it
# depends on how Fit chooses to output the name of that value
RESULTS_TABLE_COLUMNS_NO_ERRS = ['Cost function value']


class ResultsTabModel(object):
    """Captures the data and operations
    for the results tab"""

    def __init__(self, fitting_context):
        """
        Construct a model around the given FitContext object

        :param fitting_context: A FitContext detailing the outputs from the
        fitting
        """
        self._fit_context = fitting_context
        self._results_table_name = DEFAULT_TABLE_NAME
        self._selected_fit_function = None

    def results_table_name(self):
        """Return the current name of the results table"""
        return self._results_table_name

    def set_results_table_name(self, name):
        """Set the name of the output table

        :param name: A new name for the table
        """
        self._results_table_name = name

    def selected_fit_function(self):
        """Return the name of the selected fit function"""
        return self._selected_fit_function

    def set_selected_fit_function(self, name):
        """Set the name of the selected fit_function

        :param name: The name of the selected function
        """
        self._selected_fit_function = name

    def fit_functions(self):
        """
        :return: The list of fit functions known to have been fitted
        """
        return self._fit_context.fit_function_names()

    def fit_selection(self, existing_selection):
        """
        Combine the existing selection state of workspaces with the workspace names
        of fits stored here. New workspaces are always checked for inclusion.
        :param existing_selection: A dict defining any current selection model. The
        format matches that of the ListSelectorPresenter class' model.
        :return: The workspaces that have had fits performed on them along with their selection status. The
        format matches that of the ListSelectorPresenter class' model.
        """
        selection = {}
        for index, fit in enumerate(self._fit_context.fit_list):
            name = fit.input_workspace
            if name in existing_selection:
                checked = existing_selection[name][1]
            else:
                checked = True
            selection[name] = [index, checked, True]

        return selection

    def log_selection(self, existing_selection):
        """
        Combine the existing selection state of log values with the the set for the current
        workspace at the top of the list
        :param existing_selection: A dict defining any current selection model. The
        format matches that of the ListSelectorPresenter class' model.
        :return: The logs in the first workspace along with their selection status. The
        format matches that of the ListSelectorPresenter class' model.
        """
        selection = {}
        fits = self._fit_context.fit_list
        if not fits:
            return selection

        logs = log_names(fits[0].input_workspace)
        for index, name in enumerate(logs):
            if name in existing_selection:
                checked = existing_selection[name][1]
            else:
                checked = False
            selection[name] = [index, checked, True]

        return selection

    # Ideally log_selection and model_selection would be part of this model but the ListSelectorPresenter
    # contains the model and it's not yet trivial to share that
    def create_results_table(self, log_selection, results_selection):
        """Create a TableWorkspace with the fit results and logs combined. The format is
        a single row per workspace with columns:
          |workspace_name|selected_log_1|selected_log_2|...|param1_|param1_err|param2|param2_err|...|
        Any time-series log values are averaged.
        The workspace is added to the ADS with the name given by results_table_name

        :param log_selection: The current selection of logs as a list
        It is assumed this is ordered as it should be displayed. It can be empty.
        :param results_selection: The current selection of result workspaces as a list of 2-tuple
        [(workspace, fit_position),...]
        It is assumed this is not empty and ordered as it should be displayed.
        """
        results_table = self._create_empty_results_table(
            log_selection, results_selection)

        ads = AnalysisDataService.Instance()
        fit_list = self._fit_context.fit_list
        for _, position in results_selection:
            fit = fit_list[position]
            row_dict = {'workspace_name': fit.parameter_name}
            # logs first
            workspace = ads[fit.input_workspace]
            ws_run = workspace.run()
            for log_name in log_selection:
                try:
                    log_value = ws_run.getPropertyAsSingleValue(log_name)
                except Exception:
                    log_value = np.nan
                row_dict.update({log_name: log_value})
            # fit parameters
            parameter_dict = fit.parameter_workspace.workspace.toDict()
            for name, value, error in zip(parameter_dict['Name'],
                                          parameter_dict['Value'],
                                          parameter_dict['Error']):
                row_dict.update({name: value})
                if name not in RESULTS_TABLE_COLUMNS_NO_ERRS:
                    row_dict.update({name + 'Error': error})

            results_table.addRow(row_dict)

        AnalysisDataService.Instance().addOrReplace(self.results_table_name(),
                                                    results_table)
        return results_table

    def _create_empty_results_table(self, log_selection, results_selection):
        """
        Create an empty table workspace to store the results.
        :param log_selection: See create_results_table
        :param results_selection: See create_results_table
        :return: A new TableWorkspace
        """
        table = WorkspaceFactory.Instance().createTable()
        table.addColumn('str', 'workspace_name')
        for log_name in log_selection:
            table.addColumn('float', log_name)
        # assume all fit functions are the same in fit_selection and take
        # the parameter names from the first fit.
        parameters = self._first_fit_parameters(results_selection)
        for name in parameters['Name']:
            table.addColumn('float', name)
            if name not in RESULTS_TABLE_COLUMNS_NO_ERRS:
                table.addColumn('float', name + 'Error')
        return table

    # Private API
    def _first_fit_parameters(self, results_selection):
        """
        Return the first fit in the selected list.

        :param results_selection: The list of selected results
        :return: The parameters of the first fit
        """
        first_fit = self._fit_context.fit_list[results_selection[0][1]]
        return first_fit.parameter_workspace.workspace.toDict()


# Public helper functions
def log_names(workspace_name):
    """
    Return a list of log names from the given workspace.

    :param workspace: A string name of a workspace in the ADS
    :return: A list of sample log names
    :raises KeyError: if the workspace does not exist in the ADS
    """
    all_logs = AnalysisDataService.retrieve(workspace_name).run().getLogData()
    return [log.name for log in all_logs if _log_should_be_displayed(log)]


# Private helper functions
def _log_should_be_displayed(log):
    """Returns true if the given log should be included in the display"""
    return isinstance(log, FloatTimeSeriesProperty) or \
           log.name in ALLOWED_NON_TIME_SERIES_LOGS
