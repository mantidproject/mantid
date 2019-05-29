# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

from mantid.api import AnalysisDataService
from mantid.kernel import FloatTimeSeriesProperty

# Constants
DEFAULT_TABLE_NAME = 'ResultsTable'
ALLOWED_NON_TIME_SERIES_LOGS = ("run_number", "run_start", "run_end", "group",
                                "period", "sample_temp", "sample_magn_field")


class ResultsTabModel(object):
    """Captures the data and operations
    for the results tab"""

    def __init__(self, fitting_context):
        """
        Construct a model around the given FitContext object

        :param fitting_context: A FitContext detailing the outputs from the
        fitting
        """
        self._results_table_name = DEFAULT_TABLE_NAME
        self._fit_context = fitting_context

    def results_table_name(self):
        """Return the current name of the results table"""
        return self._results_table_name

    def set_results_table_name(self, name):
        """Set the name of the output table

        :param name: A new name for the table
        """
        self._results_table_name = name

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


# Public helper functions
def log_names(workspace_name):
    """
    Return a list of log names from the given workspace.

    :param workspace: A string name of a workspace in the ADS
    :return: A list of sample log names
    :raises KeyError: if the workspace does not exist in the ADS
    """
    all_logs = AnalysisDataService.retrieve(
        workspace_name).run().getLogData()
    return [log.name for log in all_logs if _log_should_be_displayed(log)]

# Private helper functions
def _log_should_be_displayed(log):
    """Returns true if the given log should be included in the display"""
    return isinstance(log, FloatTimeSeriesProperty) or \
           log.name in ALLOWED_NON_TIME_SERIES_LOGS
