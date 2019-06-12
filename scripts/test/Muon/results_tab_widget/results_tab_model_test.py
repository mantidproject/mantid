# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, print_function, unicode_literals)

from collections import OrderedDict
from copy import deepcopy
import unittest

from mantid.api import AnalysisDataService, ITableWorkspace, WorkspaceFactory, WorkspaceGroup
from mantid.kernel import FloatTimeSeriesProperty, StringPropertyWithValue
from mantid.py3compat import iteritems, mock, string_types

from Muon.GUI.Common.results_tab_widget.results_tab_model import (
    DEFAULT_TABLE_NAME, ResultsTabModel, TableColumnType)
from Muon.GUI.Common.contexts.fitting_context import FittingContext, FitInformation


def create_test_workspace(ws_name=None):
    fake_ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
    ws_name = ws_name if ws_name is not None else 'results_tab_model_test'
    AnalysisDataService.Instance().addOrReplace(ws_name, fake_ws)
    return fake_ws


def create_test_fits(input_workspaces,
                     function_name,
                     parameters,
                     output_workspace_names,
                     global_parameters=None):
    """
    Create a list of fits
    :param input_workspaces: The input workspaces
    :param function_name: The name of the function
    :param parameters: The parameters list
    :param output_workspace_names: A list of workspace names
    :param global_parameters: An optional list of tied parameters
    :return: A list of Fits
    """
    # Convert parameters to fit table-like structure
    fit_table = [{
        'Name': name,
        'Value': value,
        'Error': error
    } for name, (value, error) in iteritems(parameters)]

    fits = []
    for name in input_workspaces:
        parameter_workspace = mock.NonCallableMagicMock()
        parameter_workspace.workspace.__iter__.return_value = fit_table
        parameter_workspace.workspace_name = name + '_Parameters'
        fits.append(
            FitInformation(parameter_workspace, function_name, name, output_workspace_names,
                           global_parameters))

    return fits


def create_test_model(input_workspaces,
                      function_name,
                      parameters,
                      output_workspace_names,
                      logs=None,
                      global_parameters=None):
    """
    Create a list of fits with time series logs on the workspaces
    :param input_workspaces: See create_test_fits
    :param function_name: See create_test_fits
    :param parameters: See create_test_fits
    :param logs: A list of (name, (values...), (name, (values...)))
    :param global_parameters: An optional list of tied parameters
    :return: A list of Fits with workspaces/logs attached
    """
    fits = create_test_fits(input_workspaces, function_name, parameters, output_workspace_names,
                            global_parameters)
    logs = logs if logs is not None else []
    for fit, workspace_name in zip(fits, input_workspaces):
        add_logs(workspace_name, logs)

    fitting_context = FittingContext()
    for fit in fits:
        fitting_context.add_fit(fit)
    return fitting_context, ResultsTabModel(fitting_context)


def add_logs(workspace_name, logs):
    """
    Add a list of logs to a workspace
    :param workspace_name: A workspace to contain the logs
    :param logs: A list of logs and values
    :return: The workspace reference
    """
    workspace = create_test_workspace(workspace_name)

    run = workspace.run()
    # populate with log data
    for name, values in logs:
        tsp = FloatTimeSeriesProperty(name)
        for value in values:
            tsp.addValue("2019-05-30T09:00:00", float(value))
        run.addProperty(name, tsp, replace=True)

    return workspace


class ResultsTabModelTest(unittest.TestCase):
    def setUp(self):
        self.f0_height = (2309.2, 16)
        self.f0_centre = (2.1, 0.002)
        self.f0_sigma = (1.1, 0.001)
        self.f1_height = (2315.2, 14)
        self.f1_centre = (2.5, 0.004)
        self.f1_sigma = (0.9, 0.002)
        self.cost_function = (30.8, 0)
        self.parameters = OrderedDict([('f0.Height', self.f0_height),
                                       ('f0.PeakCentre', self.f0_centre),
                                       ('f0.Sigma', self.f0_sigma),
                                       ('f1.Height', self.f1_height),
                                       ('f1.PeakCentre', self.f1_centre),
                                       ('f1.Sigma', self.f1_sigma),
                                       ('Cost function value',
                                        self.cost_function)])

        self.log_names = ['sample_temp', 'sample_magn_field']
        self.logs = [(self.log_names[0], (50., 60.)),
                     (self.log_names[1], (2., 3.))]

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    # ------------------------- success tests ----------------------------
    def test_default_model_has_results_table_name(self):
        model = ResultsTabModel(FittingContext())
        self.assertEqual(model.results_table_name(), DEFAULT_TABLE_NAME)

    def test_updating_model_results_table_name(self):
        table_name = 'table_name'
        model = ResultsTabModel(FittingContext())
        model.set_results_table_name(table_name)

        self.assertEqual(model.results_table_name(), table_name)

    def test_default_model_has_no_selected_function_without_fits(self):
        model = ResultsTabModel(FittingContext())

        self.assertTrue(model.selected_fit_function() is None)

    def test_updating_model_selected_fit_function(self):
        model = ResultsTabModel(FittingContext())
        new_selection = 'func2'
        model.set_selected_fit_function(new_selection)

        self.assertEqual(model.selected_fit_function(), new_selection)

    def test_model_returns_fit_functions_from_context(self):
        _, model = create_test_model(('ws1', ), 'func1', self.parameters, [],
                                     self.logs)

        self.assertEqual(['func1'], model.fit_functions())

    def test_model_returns_no_fit_selection_if_no_fits_present(self):
        model = ResultsTabModel(FittingContext())
        self.assertEqual(0, len(model.fit_selection({})))

    def test_model_creates_fit_selection_given_no_existing_state(self):
        _, model = create_test_model(('ws1', 'ws2'), 'func1', self.parameters, [],
                                     self.logs)

        expected_list_state = {
            'ws1_Parameters': [0, True, True],
            'ws2_Parameters': [1, True, True]
        }
        self.assertDictEqual(expected_list_state, model.fit_selection({}))

    def test_model_creates_fit_selection_given_existing_state(self):
        _, model = create_test_model(('ws1', 'ws2'), 'func1', self.parameters, [],
                                     self.logs)

        orig_list_state = {'ws1_Parameters': [0, False, True]}
        expected_list_state = {
            'ws1_Parameters': [0, False, True],
            'ws2_Parameters': [1, True, True]
        }
        self.assertEqual(expected_list_state,
                         model.fit_selection(orig_list_state))

    def test_model_returns_no_log_selection_if_no_fits_present(self):
        model = ResultsTabModel(FittingContext())
        self.assertEqual(0, len(model.log_selection({})))

    def test_model_combines_existing_log_selection(self):
        _, model = create_test_model(('ws1', ), 'func1', self.parameters)
        model._fit_context.log_names = mock.MagicMock()
        model._fit_context.log_names.return_value = [
            'run_number', 'run_start', 'magnetic_field'
        ]

        existing_selection = {
            'run_number': [0, False, True],
            'run_start': [1, True, True],
        }
        expected_selection = deepcopy(existing_selection)
        expected_selection.update({
            'run_number': [0, False, True],
            'run_start': [1, True, True],
            'magnetic_field': [2, False, True],
        })

        self.assertDictEqual(expected_selection,
                             model.log_selection(existing_selection))

    def test_create_results_table_with_no_logs_or_global_parameters(self):
        _, model = create_test_model(('ws1', ), 'func1', self.parameters, [])
        logs = []
        selected_results = [('ws1', 0)]
        table = model.create_results_table(logs, selected_results)

        expected_cols = [
            'workspace_name', 'f0.Height', 'f0.HeightError', 'f0.PeakCentre',
            'f0.PeakCentreError', 'f0.Sigma', 'f0.SigmaError', 'f1.Height',
            'f1.HeightError', 'f1.PeakCentre', 'f1.PeakCentreError',
            'f1.Sigma', 'f1.SigmaError', 'Cost function value'
        ]
        expected_types = (TableColumnType.NoType, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y)
        expected_content = [
            ('ws1_Parameters', self.f0_height[0], self.f0_height[1],
             self.f0_centre[0], self.f0_centre[1], self.f0_sigma[0],
             self.f0_sigma[1], self.f1_height[0], self.f1_height[1],
             self.f1_centre[0], self.f1_centre[1], self.f1_sigma[0],
             self.f1_sigma[1], self.cost_function[0])
        ]
        self._assert_table_matches_expected(zip(expected_cols, expected_types),
                                            expected_content, table,
                                            model.results_table_name())

    def test_create_results_table_with_logs_selected(self):
        _, model = create_test_model(('ws1', ), 'func1', self.parameters, [],
                                     self.logs)
        selected_results = [('ws1', 0)]
        table = model.create_results_table(self.log_names, selected_results)

        expected_cols = ['workspace_name'] + self.log_names + [
            'f0.Height', 'f0.HeightError', 'f0.PeakCentre',
            'f0.PeakCentreError', 'f0.Sigma', 'f0.SigmaError', 'f1.Height',
            'f1.HeightError', 'f1.PeakCentre', 'f1.PeakCentreError',
            'f1.Sigma', 'f1.SigmaError', 'Cost function value'
        ]
        expected_types = (TableColumnType.NoType, TableColumnType.X,
                          TableColumnType.X, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y)
        avg_log_values = 55., 2.5
        expected_content = [
            ('ws1_Parameters', avg_log_values[0], avg_log_values[1],
             self.f0_height[0], self.f0_height[1], self.f0_centre[0],
             self.f0_centre[1], self.f0_sigma[0], self.f0_sigma[1],
             self.f1_height[0], self.f1_height[1], self.f1_centre[0],
             self.f1_centre[1], self.f1_sigma[0], self.f1_sigma[1],
             self.cost_function[0])
        ]
        self._assert_table_matches_expected(zip(expected_cols, expected_types),
                                            expected_content, table,
                                            model.results_table_name())

    def test_create_results_table_with_fit_with_global_parameters(self):
        logs = []
        global_parameters = ['Height']
        _, model = create_test_model(('simul-1', ), 'func1', self.parameters, [],
                                     logs, global_parameters)
        selected_results = [('simul-1', 0)]
        table = model.create_results_table(logs, selected_results)

        expected_cols = [
            'workspace_name', 'Height', 'HeightError', 'f0.PeakCentre',
            'f0.PeakCentreError', 'f0.Sigma', 'f0.SigmaError', 'f1.PeakCentre',
            'f1.PeakCentreError', 'f1.Sigma', 'f1.SigmaError',
            'Cost function value'
        ]
        expected_types = (TableColumnType.NoType, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y,
                          TableColumnType.YErr, TableColumnType.Y)
        expected_content = [
            ('simul-1_Parameters', self.f0_height[0], self.f0_height[1],
             self.f0_centre[0], self.f0_centre[1], self.f0_sigma[0],
             self.f0_sigma[1], self.f1_centre[0], self.f1_centre[1],
             self.f1_sigma[0], self.f1_sigma[1], self.cost_function[0])
        ]
        self._assert_table_matches_expected(zip(expected_cols, expected_types),
                                            expected_content, table,
                                            model.results_table_name())

    # ------------------------- failure tests ----------------------------
    def test_create_results_table_raises_error_if_number_params_different(
            self):
        parameters = OrderedDict([('Height', (100, 0.1)),
                                  ('Cost function value', (1.5, 0))])
        fits_func1 = create_test_fits(('ws1', ), 'func1', parameters, [])

        parameters = OrderedDict([('Height', (100, 0.1)), ('A0', (1, 0.001)),
                                  ('Cost function value', (1.5, 0))])
        fits_func2 = create_test_fits(('ws2', ), 'func2', parameters, [])
        model = ResultsTabModel(FittingContext(fits_func1 + fits_func2))

        selected_results = [('ws1', 0), ('ws2', 1)]
        self.assertRaises(RuntimeError, model.create_results_table, [],
                          selected_results)

    def test_create_results_table_with_mixed_global_non_global_raises_error(
            self):
        parameters = OrderedDict([('f0.Height', (100, 0.1)),
                                  ('f1.Height', (90, 0.001)),
                                  ('Cost function value', (1.5, 0))])
        fits_func1= create_test_fits(('ws1', ), 'func1', parameters, [])
        fits_globals = create_test_fits(('ws2', ), 'func1', parameters, [],
                                        global_parameters=['Height'])
        model = ResultsTabModel(FittingContext(fits_func1 + fits_globals))

        selected_results = [('ws1', 0), ('ws2', 1)]
        self.assertRaises(RuntimeError, model.create_results_table, [],
                          selected_results)

    def test_create_results_table_with_logs_missing_from_some_workspaces_raises(
            self):
        parameters = OrderedDict([('f0.Height', (100, 0.1))])
        logs = [('log1', (1., 2.)), ('log2', (3., 4.)), ('log3', (4., 5.)),
                ('log4', (5., 6.))]
        fits_logs1 = create_test_fits(('ws1', ), 'func1', parameters)
        add_logs(fits_logs1[0].input_workspaces[0], logs[:2])

        fits_logs2 = create_test_fits(('ws2', ), 'func1', parameters)
        add_logs(fits_logs2[0].input_workspaces[0], logs[2:])
        model = ResultsTabModel(FittingContext(fits_logs1 + fits_logs2))

        selected_results = [('ws1', 0), ('ws2', 1)]
        selected_logs = ['log1', 'log3']
        self.assertRaises(RuntimeError, model.create_results_table,
                          selected_logs, selected_results)

    # ---------------------- Private helper functions -------------------------

    def _assert_table_matches_expected(self, expected_cols, expected_content,
                                       table, table_name):
        self.assertTrue(isinstance(table, ITableWorkspace))
        self.assertTrue(table_name in AnalysisDataService.Instance())
        self.assertEqual(len(expected_content), table.rowCount())
        self.assertEqual(len(expected_cols), table.columnCount())
        actual_col_names = table.getColumnNames()
        for index, (expected_name, expected_type) in enumerate(expected_cols):
            self.assertEqual(expected_name, actual_col_names[index])
            self.assertEqual(expected_type.value, table.getPlotType(index))

        for row_index, (expected_row,
                        actual_row) in enumerate(zip(expected_content, table)):
            self.assertEqual(len(expected_row), len(actual_row))
            for col_index, expected in enumerate(expected_row):
                actual = table.cell(row_index, col_index)
                if isinstance(expected, string_types):
                    self.assertEqual(expected, actual)
                else:
                    # Fit pushes things back/forth through strings so exact match is not possible
                    self.assertAlmostEqual(expected, actual, places=3)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
