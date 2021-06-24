# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import datetime
import unittest
from collections import OrderedDict
from copy import deepcopy
from unittest import mock

from Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FitInformation
from Muon.GUI.Common.contexts.fitting_contexts.tf_asymmetry_fitting_context import TFAsymmetryFittingContext
from Muon.GUI.Common.contexts.results_context import ResultsContext
from Muon.GUI.Common.results_tab_widget.results_tab_model import (
    DEFAULT_TABLE_NAME, ResultsTabModel, TableColumnType)
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper
from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import AnalysisDataService, ITableWorkspace, WorkspaceFactory
from mantid.kernel import FloatTimeSeriesProperty
from mantid.simpleapi import Load


def create_test_workspace(ws_name=None):
    ws_name = ws_name if ws_name is not None else 'results_tab_model_test'
    Load("MUSR22725", OutputWorkspace=ws_name)
    return AnalysisDataService.Instance().retrieve(ws_name)


def create_test_fits(input_workspaces,
                     function_name,
                     parameters,
                     output_workspace_names=None,
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
    workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)

    output_workspace_names = output_workspace_names if output_workspace_names is not None else [
        StaticWorkspaceWrapper('test-output-ws', workspace)
    ]
    # Convert parameters to fit table-like structure
    fit_table = [{
        'Name': name,
        'Value': value,
        'Error': error
    } for name, (value, error) in parameters.items()]

    fits = []
    for name in input_workspaces:
        parameter_workspace = mock.NonCallableMagicMock()
        parameter_workspace.workspace.__iter__.return_value = fit_table
        parameter_workspace.workspace_name = name + '_Parameters'
        fits.append(
            FitInformation([name], function_name, output_workspace_names, parameter_workspace, mock.Mock(),
                           global_parameters))

    return fits


def create_test_model(input_workspaces,
                      function_name,
                      parameters,
                      output_workspace_names=None,
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
    fits = create_test_fits(input_workspaces, function_name, parameters,
                            output_workspace_names, global_parameters)
    logs = logs if logs is not None else []
    for fit, workspace_name in zip(fits, input_workspaces):
        add_logs(workspace_name, logs)

    fitting_context = TFAsymmetryFittingContext()
    for fit in fits:
        fitting_context.add_fit(fit)
    return fitting_context, ResultsTabModel(fitting_context, ResultsContext())


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
    dt_format = "%Y-%m-%dT%H:%M:%S"
    for name, values in logs:
        tsp = FloatTimeSeriesProperty(name)
        time = datetime.datetime.strptime("2019-05-30T09:00:00", dt_format)
        for value in values:
            tsp.addValue(time.strftime(dt_format), float(value))
            time += datetime.timedelta(seconds=5)
        run.addProperty(name, tsp, replace=True)

    return workspace


@start_qapplication
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
        model = ResultsTabModel(TFAsymmetryFittingContext(), ResultsContext())
        self.assertEqual(model.results_table_name(), DEFAULT_TABLE_NAME)

    def test_updating_model_results_table_name(self):
        table_name = 'table_name'
        model = ResultsTabModel(TFAsymmetryFittingContext(), ResultsContext())
        model.set_results_table_name(table_name)

        self.assertEqual(model.results_table_name(), table_name)

    def test_default_model_has_no_selected_function_without_fits(self):
        model = ResultsTabModel(TFAsymmetryFittingContext(), ResultsContext())

        self.assertTrue(model.selected_fit_function() is None)

    def test_updating_model_selected_fit_function(self):
        model = ResultsTabModel(TFAsymmetryFittingContext(), ResultsContext())
        new_selection = 'func2'
        model.set_selected_fit_function(new_selection)

        self.assertEqual(model.selected_fit_function(), new_selection)

    def test_model_returns_fit_functions_from_context(self):
        _, model = create_test_model(['ws1'], 'func1', self.parameters, [], self.logs)

        self.assertEqual(['func1'], model.fit_functions())

    def test_model_returns_no_fit_selection_if_no_fits_present(self):
        model = ResultsTabModel(TFAsymmetryFittingContext(), ResultsContext())
        self.assertEqual(0, len(model.fit_selection({})))

    def test_model_creates_fit_selection_given_no_existing_state(self):
        _, model = create_test_model(('ws1', 'ws2'), 'func1', self.parameters, logs=self.logs)

        expected_list_state = {
            'ws1_Parameters': [0, False, True],
            'ws2_Parameters': [1, False, True]
        }
        self.assertDictEqual(expected_list_state, model.fit_selection({}))

    def test_model_creates_fit_selection_given_existing_state(self):
        _, model = create_test_model(('ws1', 'ws2'), 'func1', self.parameters, logs=self.logs)

        orig_list_state = ["ws2_Parameters"]
        expected_list_state = {
            'ws1_Parameters': [0, False, True],
            'ws2_Parameters': [1, True, True]
        }
        self.assertEqual(expected_list_state,
                         model.fit_selection(orig_list_state))

    def test_model_returns_no_log_selection_if_no_fits_present(self):
        model = ResultsTabModel(TFAsymmetryFittingContext(), ResultsContext())
        self.assertEqual(0, len(model.log_selection({})))

    def test_model_combines_existing_log_selection(self):
        _, model = create_test_model(('ws1',), 'func1', self.parameters)
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
        _, model = create_test_model(('ws1',), 'func1', self.parameters)
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
        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        workspace.mutableRun().addProperty("sample_temp", 50, True)
        workspace.mutableRun().addProperty("sample_magn_field", 2, True)
        _, model = create_test_model(('ws1',), 'func1', self.parameters, [StaticWorkspaceWrapper('ws1', workspace)],
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
        avg_log_values = 50., 2.0
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
        _, model = create_test_model(('simul-1',), 'func1', self.parameters, logs=logs, global_parameters=global_parameters)
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
        fits_func1 = create_test_fits(('ws1',), 'func1', parameters)

        parameters = OrderedDict([('Height', (100, 0.1)), ('A0', (1, 0.001)),
                                  ('Cost function value', (1.5, 0))])
        fits_func2 = create_test_fits(('ws2',), 'func2', parameters)

        fitting_context = TFAsymmetryFittingContext()
        fitting_context.fit_list = fits_func1 + fits_func2
        model = ResultsTabModel(fitting_context, ResultsContext())

        selected_results = [('ws1', 0), ('ws2', 1)]
        self.assertRaises(IndexError, model.create_results_table, [],
                          selected_results)

    def test_create_results_table_with_mixed_global_non_global_raises_error(
            self):
        parameters = OrderedDict([('f0.Height', (100, 0.1)),
                                  ('f1.Height', (90, 0.001)),
                                  ('Cost function value', (1.5, 0))])
        fits_func1 = create_test_fits(('ws1',), 'func1', parameters)
        fits_globals = create_test_fits(('ws2',),
                                        'func1',
                                        parameters,
                                        global_parameters=['Height'])

        fitting_context = TFAsymmetryFittingContext()
        fitting_context.fit_list = fits_func1 + fits_globals
        model = ResultsTabModel(fitting_context, ResultsContext())

        selected_results = [('ws1', 0), ('ws2', 1)]
        self.assertRaises(IndexError, model.create_results_table, [],
                          selected_results)

    def test_create_results_table_with_logs_missing_from_some_workspaces_raises(self):
        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)

        parameters = OrderedDict([('f0.Height', (100, 0.1))])
        logs = [('log1', (1., 2.)), ('log2', (3., 4.)), ('log3', (4., 5.)),
                ('log4', (5., 6.))]
        fits_logs1 = create_test_fits(('ws1',), 'func1', parameters,
                                      output_workspace_names=[StaticWorkspaceWrapper('test-ws1-ws', workspace)])
        add_logs(fits_logs1[0].input_workspaces[0], logs[:2])

        fits_logs2 = create_test_fits(('ws2',), 'func1', parameters,
                                      output_workspace_names=[StaticWorkspaceWrapper('test-ws2-ws', workspace)])
        add_logs(fits_logs2[0].input_workspaces[0], logs[2:])

        fitting_context = TFAsymmetryFittingContext()
        fitting_context.fit_list = fits_logs1 + fits_logs2
        model = ResultsTabModel(fitting_context, ResultsContext())

        selected_results = [('ws1', 0), ('ws2', 1)]
        selected_logs = ['log1', 'log3']
        self.assertRaises(IndexError, model.create_results_table,
                          selected_logs, selected_results)

    def test_that_when_new_fit_is_performed_function_name_is_set_to_lastest_fit_name(self):
        parameters = OrderedDict([('Height', (100, 0.1)),
                                  ('Cost function value', (1.5, 0))])
        fits_func1 = create_test_fits(('ws1',), 'func1', parameters)

        parameters = OrderedDict([('Height', (100, 0.1)), ('A0', (1, 0.001)),
                                  ('Cost function value', (1.5, 0))])
        fits_func2 = create_test_fits(('ws2',), 'func2', parameters)

        fitting_context = TFAsymmetryFittingContext()
        fits = fits_func1 + fits_func2
        for fit in fits:
            fitting_context.add_fit(fit)
        model = ResultsTabModel(fitting_context, ResultsContext())

        model.on_new_fit_performed()

        self.assertEqual(model.selected_fit_function(), 'func2')

    # ---------------------- Private helper functions -------------------------

    def _assert_table_matches_expected(self, expected_cols, expected_content,
                                       table, table_name):
        self.assertTrue(isinstance(table, ITableWorkspace))
        self.assertTrue(table_name in AnalysisDataService.Instance())
        self.assertEqual(len(expected_content), table.rowCount())
        self.assertEqual(len(list(expected_cols)), table.columnCount())
        actual_col_names = table.getColumnNames()
        for index, (expected_name, expected_type) in enumerate(expected_cols):
            self.assertEqual(expected_name, actual_col_names[index])
            self.assertEqual(expected_type.value, table.getPlotType(index))

        for row_index, (expected_row,
                        actual_row) in enumerate(zip(expected_content, table)):
            self.assertEqual(len(expected_row), len(actual_row))
            for col_index, expected in enumerate(expected_row):
                actual = table.cell(row_index, col_index)
                if isinstance(expected, str):
                    self.assertEqual(expected, actual)
                else:
                    # Fit pushes things back/forth through strings so exact match is not possible
                    self.assertAlmostEqual(expected, actual, places=3)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
