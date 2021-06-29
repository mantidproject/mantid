# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from mantid.api import AnalysisDataService as ads, WorkspaceFactory
from mantid.kernel import FloatTimeSeriesProperty
from enum import Enum

# Constants
DEFAULT_TABLE_NAME = 'ResultsTable'
ALLOWED_NON_TIME_SERIES_LOGS = ("run_number", "run_start", "run_end", "group",
                                "period", "sample_temp", "sample_magn_field")
ERROR_COL_SUFFIX = 'Error'
# This is not a particularly robust way of ignoring this as it
# depends on how Fit chooses to output the name of that value
RESULTS_TABLE_COLUMNS_NO_ERRS = ['Cost function value']
WORKSPACE_NAME_COL = 'workspace_name'


class TableColumnType(Enum):
    """Enumeration to match the expected int used for TableWorkspace.addColumn
    for specifying the column type"""

    NotSet = -1000
    NoType = 0
    X = 1
    Y = 2
    Z = 3
    XErr = 4
    YErr = 5
    Label = 6


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

        self._update_selected_fit_function()

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

    def fit_selection(self, to_select):
        """
        Combine the existing selection state of workspaces with the workspace names
        of fits stored here. New workspaces are always checked for inclusion.
        :param to_select A list of workspace names to select
        :return: The workspaces that have had fits performed on them along with their selection status. The
        format matches that of the ListSelectorPresenter class' model.
        """
        selection = {}
        for index, fit in enumerate(self._fit_context.fit_list):
            if fit.fit_function_name != self.selected_fit_function():
                continue
            name = fit.parameters.parameter_workspace_name
            selection[name] = [index, name in to_select, True]

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
        logs = self._fit_context.log_names(filter_fn=_log_should_be_displayed)
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
        self._raise_error_on_incompatible_selection(log_selection,
                                                    results_selection)
        all_fits = self._fit_context.fit_list
        results_table = self._create_empty_results_table(
            log_selection, results_selection, all_fits)
        for _, position in results_selection:
            fit = all_fits[position]
            fit_parameters = fit.parameters
            row_dict = {
                WORKSPACE_NAME_COL: fit_parameters.parameter_workspace_name
            }
            row_dict = self._add_logs_to_table(row_dict, fit, log_selection)
            results_table.addRow(
                self._add_parameters_to_table(row_dict, fit_parameters))

        ads.Instance().addOrReplace(self.results_table_name(), results_table)
        return results_table

    def _add_logs_to_table(self, row_dict, fit, log_selection):
        """
        Add the log values into the row for the given fit
        :param row_dict: The dict of current row values
        :param fit: The fit object being processed
        :param log_selection: The current selection of logs as a list of names
        :return: The updated row values dict
        """
        if not log_selection:
            return row_dict

        for log_name in log_selection:
            row_dict.update({log_name: fit.log_value(log_name)})

        return row_dict

    def _add_parameters_to_table(self, row_dict, fit_parameters):
        """
        Add the parameter values into the row for the given fit
        :param row_dict: The dict of current row values
        :param fit_parameters: The list of FitParameter objects
        :return: The updated row dictionary
        """
        for param_name in fit_parameters.names():
            row_dict.update({param_name: fit_parameters.value(param_name)})
            if _param_error_should_be_displayed(param_name):
                row_dict.update({
                    _error_column_name(param_name):
                    fit_parameters.error(param_name)
                })

        return row_dict

    def _raise_error_on_incompatible_selection(self, log_selection,
                                               results_selection):
        """If the selected results cannot be displayed together then raise an error

        :param log_selection: See create_results_output
        :param results_selection: See create_results_output
        :raises RuntimeError if the selection cannot produce a valid table
        """
        self._raise_if_log_selection_invalid(log_selection, results_selection)
        self._raise_if_result_selection_is_invalid(results_selection)

    def _raise_if_log_selection_invalid(self, log_selection,
                                        results_selection):
        """
        Raise a RuntimeError if the log selection is invalid.
        :param results_selection: The selected fit results
        :param results_selection: The selected log values
        """
        all_fits = self._fit_context.fit_list
        missing_msg = []
        for selection in results_selection:
            fit = all_fits[selection[1]]
            missing = []
            for log_name in log_selection:
                if not fit.has_log(log_name):
                    missing.append(log_name)
            if missing:
                missing_msg.append("  Fit '{}' is missing the logs {}".format(
                    fit.parameters.parameter_workspace_name, missing))
        if missing_msg:
            raise RuntimeError("The logs for each selected fit do not match:\n" + "\n".join(missing_msg))

    def _raise_if_result_selection_is_invalid(self, results_selection):
        """
        Raise a RuntimeError if the result selection is invalid.
        :param results_selection: The selected fit results
        """
        all_fits = self._fit_context.fit_list
        nparams_selected = [
            len(all_fits[position].parameters)
            for _, position in results_selection
        ]
        if nparams_selected[1:] != nparams_selected[:-1]:
            msg = "The number of parameters for each selected fit does not match:\n"
            for (_, position), nparams in zip(results_selection,
                                              nparams_selected):
                fit = all_fits[position]
                msg += "  {}: {}\n".format(
                    fit.parameters.parameter_workspace_name, nparams)
            raise RuntimeError(msg)

    def _create_empty_results_table(self, log_selection, results_selection, all_fits):
        """
        Create an empty table workspace to store the results.
        :param log_selection: See create_results_table
        :param results_selection: See create_results_table
        :return: A new TableWorkspace
        """
        table = WorkspaceFactory.Instance().createTable()
        table.addColumn('str', 'workspace_name', TableColumnType.NoType.value)

        def float_log(wksp_name, log_name):
            try:
                run = ads.Instance().retrieve(wksp_name).run()
                prop = run.getProperty(log_name)
                if isinstance(prop, FloatTimeSeriesProperty):
                    return True
                try:
                    float(prop.value)
                    return True
                except ValueError:
                    return False
            except (TypeError, ValueError):
                for fit in all_fits:
                    for input_workspace in fit.input_workspaces:
                        ws = ads.Instance().retrieve(input_workspace)
                        if ws.run().hasProperty(log_name):
                            return float_log(input_workspace, log_name)

        for log_name in log_selection:
            wksp_name = all_fits[0].input_workspaces[0]
            if float_log(wksp_name, log_name):
                table.addColumn('float', log_name, TableColumnType.X.value)
            else:
                table.addColumn('str', log_name, TableColumnType.X.value)
        # assume all fit functions are the same in fit_selection and take
        # the parameter names from the first fit.
        parameters = self._find_parameters_for_table(results_selection)
        for name in parameters.names():
            table.addColumn('float', name, TableColumnType.Y.value)
            if _param_error_should_be_displayed(name):
                table.addColumn('float', _error_column_name(name),
                                TableColumnType.YErr.value)
                # The error column will be the most recent one added (columnCount-1) and is corresponding value will be
                # the second to last (columnCount-2).
                table.setLinkedYCol(table.columnCount()-1, table.columnCount()-2)
        return table

    def on_new_fit_performed(self):
        """Called when a new fit has been added to the context.
        The function name is set to the name fit if it is the first time"""
        self._update_selected_fit_function()

    # Private API
    def _update_selected_fit_function(self):
        """
        If there are fits present then set the selected function name or else
        clear it
        """
        if len(self._fit_context) > 0:
            function_name = self._fit_context.fit_list[-1].fit_function_name
        else:
            function_name = None

        self.set_selected_fit_function(function_name)

    def _find_parameters_for_table(self, results_selection):
        """
        Return the parameters that should be inserted into the
        results table. This takes into account any global parameters
        that are set

        :param results_selection: The list of selected results
        :return: The parameters for the table
        """
        first_fit = self._fit_context.fit_list[results_selection[0][1]]
        return first_fit.parameters


# Private helper functions
def _log_should_be_displayed(log):
    """Returns true if the given log should be included in the display"""
    return isinstance(log, FloatTimeSeriesProperty) or \
        log.name in ALLOWED_NON_TIME_SERIES_LOGS


def _param_error_should_be_displayed(param_name):
    """Returns true if the given parameter's error should included in the display"""
    return param_name not in RESULTS_TABLE_COLUMNS_NO_ERRS


def _error_column_name(name):
    """
    Create the name of a column for an error on the named value
    :param name: The name of a value column
    :return: A name for the error column
    """
    return name + ERROR_COL_SUFFIX
