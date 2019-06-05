# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, unicode_literals)

import itertools
import unittest

from mantid.api import AnalysisDataService, ITableWorkspace, WorkspaceFactory, WorkspaceGroup
from mantid.kernel import FloatTimeSeriesProperty, StringPropertyWithValue
from mantid.py3compat import mock

from Muon.GUI.Common.results_tab_widget.results_tab_model import (
    DEFAULT_TABLE_NAME, ALLOWED_NON_TIME_SERIES_LOGS, log_names,
    ResultsTabModel)
from Muon.GUI.Common.contexts.fitting_context import FittingContext, FitInformation

# constants
LOG_NAMES_FUNC = 'Muon.GUI.Common.results_tab_widget.results_tab_model.log_names'


class ResultsTabModelTest(unittest.TestCase):
    def setUp(self):
        self.parameters = {
            'Name': ['Height', 'PeakCentre', 'Sigma', 'Cost function value'],
            'Value': [2309.2, 2.1, 0.04, 30.8],
            'Error': [16, 0.002, 0.003, 0]
        }
        self.logs = ['sample_temp', 'sample_magn_field']
        fits = create_test_fits_with_logs(('ws1', ), 'func1', self.parameters,
                                          self.logs)
        self.fitting_context = FittingContext()
        for fit in fits:
            self.fitting_context.add_fit(fit)

        self.model = ResultsTabModel(self.fitting_context)

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    # ------------------------- success tests ----------------------------
    def test_default_model_has_results_table_name(self):
        self.assertEqual(self.model.results_table_name(), DEFAULT_TABLE_NAME)

    def test_updating_model_results_table_name(self):
        table_name = 'table_name'
        self.model.set_results_table_name(table_name)
        self.assertEqual(self.model.results_table_name(), table_name)

    def test_default_model_has_no_selected_function_without_fits(self):
        model = ResultsTabModel(FittingContext())

        self.assertTrue(model.selected_fit_function() is None)

    def test_updating_model_selected_fit_function(self):
        new_selection = 'func2'
        self.model.set_selected_fit_function(new_selection)
        self.assertEqual(self.model.selected_fit_function(), new_selection)

    def test_log_names_from_workspace_with_logs(self):
        fake_ws = create_test_workspace()
        run = fake_ws.run()
        # populate with log data
        time_series_names = ('ts_1', 'ts_2')
        for name in time_series_names:
            run.addProperty(name, FloatTimeSeriesProperty(name), replace=True)
        single_value_log_names = ('sv_1', 'sv_2')
        for name in itertools.chain(single_value_log_names,
                                    ALLOWED_NON_TIME_SERIES_LOGS):
            run.addProperty(name,
                            StringPropertyWithValue(name, 'test'),
                            replace=True)
        # verify
        allowed_logs = log_names(fake_ws.name())
        for name in itertools.chain(time_series_names,
                                    ALLOWED_NON_TIME_SERIES_LOGS):
            self.assertTrue(
                name in allowed_logs,
                msg="{} not found in allowed log list".format(name))
        for name in single_value_log_names:
            self.assertFalse(name in allowed_logs,
                             msg="{} found in allowed log list".format(name))

    def test_log_names_from_workspace_without_logs(self):
        fake_ws = create_test_workspace()
        allowed_logs = log_names(fake_ws.name())
        self.assertEqual(0, len(allowed_logs))

    def test_log_names_from_workspacegroup_uses_first_workspace(self):
        def add_log(workspace, name):
            run = workspace.run()
            run.addProperty(name, FloatTimeSeriesProperty(name), replace=True)

        fake_group = create_test_workspacegroup(size=2)
        logs = ['log_1', 'log_2']
        for index, name in enumerate(logs):
            add_log(fake_group[index], name)

        visible_logs = log_names(fake_group.name())
        self.assertTrue(logs[0] in visible_logs,
                        msg="{} not found in log list".format(logs[0]))
        self.assertFalse(logs[1] in visible_logs,
                         msg="{} not found in log list".format(logs[1]))

    def test_model_returns_fit_functions_from_context(self):
        self.assertEqual(['func1'], self.model.fit_functions())

    def test_model_returns_no_fit_selection_if_no_fits_present(self):
        model = ResultsTabModel(FittingContext())
        self.assertEqual(0, len(model.fit_selection({})))

    def test_model_creates_fit_selection_given_zero_existing_state(self):
        expected_list_state = {'ws1': [0, True, True]}

        self.assertEqual(expected_list_state, self.model.fit_selection({}))

    def test_model_creates_fit_selection_given_existing_state(self):
        more_fits = create_test_fits_with_logs(('ws2', ), 'func1',
                                               self.parameters, self.logs)
        for fit in more_fits:
            self.fitting_context.add_fit(fit)

        orig_list_state = {'ws1': [0, False, True]}
        expected_list_state = {'ws1': [0, False, True], 'ws2': [1, True, True]}
        self.assertEqual(expected_list_state,
                         self.model.fit_selection(orig_list_state))

    def test_model_returns_no_log_selection_if_not_fits_present(self):
        self.fitting_context.fit_list = []
        self.assertEqual(0, len(self.model.log_selection({})))

    def test_model_returns_log_selection_of_first_workspace(self):
        self.fitting_context.fit_list = create_test_fits_with_only_workspace_names(
            ('ws1', 'ws2'))[0]
        with mock.patch(LOG_NAMES_FUNC) as mock_log_names:
            ws1_logs = ('run_number', 'run_start')
            ws2_logs = ('temp', 'magnetic_field')

            def side_effect(name):
                return ws1_logs if name == 'ws1' else ws2_logs

            mock_log_names.side_effect = side_effect
            expected_selection = {
                'run_number': [0, False, True],
                'run_start': [1, False, True],
            }

            self.assertEqual(expected_selection, self.model.log_selection({}))

    def test_model_combines_existing_selection(self):
        self.fitting_context.fit_list = create_test_fits_with_only_workspace_names(
            ('ws1', ))[0]
        with mock.patch(LOG_NAMES_FUNC) as mock_log_names:
            mock_log_names.return_value = ('run_number', 'run_start',
                                           'magnetic_field')

            existing_selection = {
                'run_number': [0, False, True],
                'run_start': [1, True, True],
            }
            expected_selection = {
                'run_number': [0, False, True],
                'run_start': [1, True, True],
                'magnetic_field': [2, False, True]
            }
            self.assertDictEqual(expected_selection,
                                 self.model.log_selection(existing_selection))

    def test_create_results_table_with_no_logs(self):
        parameters = {
            'Name': ['Height', 'PeakCentre', 'Sigma', 'Cost function value'],
            'Value': [2309.2, 2.1, 0.04, 30.8],
            'Error': [16, 0.002, 0.003, 0]
        }
        self.fitting_context.fit_list = create_test_fits(('ws1', ), 'func1',
                                                         parameters)
        selected_results = [('ws1', 0)]
        table = self.model.create_results_table([], selected_results)

        self.assertTrue(isinstance(table, ITableWorkspace))
        self.assertEqual(8, table.columnCount())
        self.assertEqual(1, table.rowCount())
        expected_cols = [
            'workspace_name', 'Height', 'HeightError', 'PeakCentre',
            'PeakCentreError', 'Sigma', 'SigmaError', 'Cost function value'
        ]
        self.assertEqual(expected_cols, table.getColumnNames())
        self.assertEqual('ws1_Parameters', table.cell(0, 0))
        for index, (expected_val, expected_err) in enumerate(
                zip(parameters['Value'], parameters['Error'])):
            self.assertAlmostEqual(expected_val,
                                   table.cell(0, 2 * index + 1),
                                   places=2)
            if 2 * index + 2 < table.columnCount():
                self.assertAlmostEqual(expected_err,
                                       table.cell(0, 2 * index + 2),
                                       places=2)

        self.assertTrue(
            self.model.results_table_name() in AnalysisDataService.Instance())

    def test_create_results_table_with_logs_selected(self):
        selected_results = [('ws1', 0)]
        logs = self.logs
        table = self.model.create_results_table(logs, selected_results)

        self.assertTrue(isinstance(table, ITableWorkspace))
        self.assertEqual(10, table.columnCount())
        self.assertEqual(1, table.rowCount())

        expected_cols = ['workspace_name'] + logs + [
            'Height', 'HeightError', 'PeakCentre', 'PeakCentreError', 'Sigma',
            'SigmaError', 'Cost function value'
        ]
        self.assertEqual(expected_cols, table.getColumnNames())

        # first value is workspace name
        self.assertEqual('ws1_Parameters', table.cell(0, 0))

        # first check log columns
        nlogs = len(logs)
        for index, _ in enumerate(logs):
            expected_val = 0.5 * (float(index) + float(index + 1))
            self.assertAlmostEqual(expected_val,
                                   table.cell(0, index + 1),
                                   places=2)
        checked_columns = 1 + nlogs
        parameters = self.parameters
        for index, (expected_val, expected_err) in enumerate(
                zip(parameters['Value'], parameters['Error'])):
            self.assertAlmostEqual(expected_val,
                                   table.cell(0, 2 * index + checked_columns),
                                   places=2)
            err_col_idx = 2 * index + checked_columns + 1
            if err_col_idx < table.columnCount():
                self.assertAlmostEqual(expected_err,
                                       table.cell(0, err_col_idx),
                                       places=2)

        self.assertTrue(
            self.model.results_table_name() in AnalysisDataService.Instance())

    # ------------------------- failure tests ----------------------------
    def test_log_names_from_workspace_not_in_ADS_raises_exception(self):
        self.assertRaises(KeyError, log_names, 'not a workspace in ADS')

    def test_create_results_table_raises_error_if_number_params_different(
            self):
        parameters = {
            'Name': ['Height', 'Cost function value'],
            'Value': [2309.2, 30.8],
            'Error': [16, 0]
        }
        fits = create_test_fits_with_logs(('ws1', ), 'func1', parameters,
                                          self.logs)
        self.fitting_context = FittingContext()
        for fit in fits:
            self.fitting_context.add_fit(fit)

        parameters = {
            'Name': ['Height', 'A0', 'Cost function value'],
            'Value': [2309.2, 0.1, 30.8],
            'Error': [16, 0.001, 0]
        }
        fits = create_test_fits_with_logs(('ws2', ), 'func1', parameters,
                                          self.logs)
        for fit in fits:
            self.fitting_context.add_fit(fit)
        self.model = ResultsTabModel(self.fitting_context)

        selected_results = [('ws1', 0), ('ws2', 1)]
        self.assertRaises(
            RuntimeError,
            self.model.create_results_table, [], selected_results)


def create_test_workspace(ws_name=None):
    fake_ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
    ws_name = ws_name if ws_name is not None else 'results_tab_model_test'
    AnalysisDataService.Instance().addOrReplace(ws_name, fake_ws)
    return fake_ws


def create_test_workspacegroup(size, group_name=None):
    group_name = group_name if group_name is not None else 'results_tab_model_testgroup'
    ads = AnalysisDataService.Instance()
    group = WorkspaceGroup()
    for i in range(size):
        fake_ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
        ws_name = '{}_{}'.format(group_name, i)
        ads.addOrReplace(ws_name, fake_ws)
        group.addWorkspace(fake_ws)

    ads.addOrReplace(group_name, group)
    return group


def create_test_fits_with_only_workspace_names(input_workspaces,
                                               checked_states=None):
    """
    Create a minimal list of fits and selection states with only
    workspace names attached to them.
    :param input_workspaces: The name of the input workspaces to create
    :param checked_states: Option set of checked states (list of boolean)
    :return: A 2-tuple of fits, selection states
    """
    fits = []
    list_state = {}
    if checked_states is None:
        checked_states = (True for _ in input_workspaces)
    enabled = True
    for index, (name,
                checked) in enumerate(zip(input_workspaces, checked_states)):
        fit_info = mock.MagicMock()
        fit_info.input_workspace = name
        fits.append(fit_info)
        list_state[name] = [index, checked, enabled]

    return fits, list_state


def create_test_fits(input_workspaces, function_name, parameters):
    """
    Create a list of fits
    :param input_workspaces: The input workspaces
    :param function_name: The name of the function
    :param parameters: The parameters list
    :return: A list of Fits
    """
    fits = []
    for name in input_workspaces:
        parameter_workspace = mock.NonCallableMagicMock()
        parameter_workspace.workspace.toDict.return_value = parameters
        parameter_workspace.workspace_name = name + '_Parameters'
        fits.append(FitInformation(parameter_workspace, function_name, name))

    return fits


def create_test_fits_with_logs(input_workspaces, function_name, parameters,
                               logs):
    """
    Create a list of fits with time series logs on the workspaces
    :param input_workspaces: See create_test_fits
    :param function_name: See create_test_fits
    :param parameters: See create_test_fits
    :param logs: A list of log names to create
    :return: A list of Fits with workspaces/logs attached
    """
    fits = create_test_fits(input_workspaces, function_name, parameters)
    for fit, workspace_name in zip(fits, input_workspaces):
        test_ws = create_test_workspace(workspace_name)
        run = test_ws.run()
        # populate with log data
        for index, name in enumerate(logs):
            tsp = FloatTimeSeriesProperty(name)
            tsp.addValue("2019-05-30T09:00:00", float(index))
            tsp.addValue("2019-05-30T09:00:05", float(index + 1))
            run.addProperty(name, tsp, replace=True)

        fit.input_workspace = workspace_name

    return fits


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
