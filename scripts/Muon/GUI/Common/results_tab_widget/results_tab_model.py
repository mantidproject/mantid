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

    def log_names(self, workspace_name):
        """
        Return a list of log names from the given workspace.

        :param workspace: A string name of a workspace in the ADS
        :return: A list of sample log names
        :raises KeyError: if the workspace does not exist in the ADS
        """
        all_logs = AnalysisDataService.retrieve(
            workspace_name).run().getLogData()
        return [log.name for log in all_logs if _log_should_be_displayed(log)]

    def fit_functions(self):
        """
        :return: The list of fit functions known to have been fitted
        """
        return self._fit_context.fit_function_names()

    def fit_input_workspaces(self):
        """
        :return: The list of workspace names of the original data used as input to the fits
        """
        return {
            fit.input_workspace: [index, True, True]
            for index, fit in enumerate(self._fit_context.fit_list)
        }


def _log_should_be_displayed(log):
    """Returns true if the given log should be included in the display"""
    return isinstance(log, FloatTimeSeriesProperty) or \
           log.name in ALLOWED_NON_TIME_SERIES_LOGS
