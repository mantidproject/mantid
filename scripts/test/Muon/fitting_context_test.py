# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from collections import OrderedDict
import unittest
from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import AnalysisDataService, WorkspaceFactory, WorkspaceGroup
from mantid.kernel import FloatTimeSeriesProperty, StringPropertyWithValue
from unittest import mock

from Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FittingContext, FitInformation, FitParameters
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper


def create_test_workspace(ws_name=None,
                          time_series_logs=None,
                          string_value_logs=None):
    """
    Create a test workspace.
    :param ws_name: An optional name for the workspace
    :param time_series_logs: A set of (name, (values,...))
    :param string_value_logs: A set of (name, value) pairs
    :return: The new workspace
    """
    fake_ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
    run = fake_ws.run()
    if time_series_logs is not None:
        for name, values in time_series_logs:
            tsp = FloatTimeSeriesProperty(name)
            for item in values:
                try:
                    time, value = item[0], item[1]
                except TypeError:
                    time, value = "2000-05-01T12:00:00", item
                tsp.addValue(time, value)
            run.addProperty(name, tsp, replace=True)

    if string_value_logs is not None:
        for name, value in string_value_logs:
            run.addProperty(
                name, StringPropertyWithValue(name, value), replace=True)

    ws_name = ws_name if ws_name is not None else 'fitting_context_model_test'
    AnalysisDataService.Instance().addOrReplace(ws_name, fake_ws)
    return fake_ws


def create_test_workspacegroup(group_name=None, size=None, items=None):
    if size is not None and items is not None:
        raise ValueError("Provide either size or items not both.")

    group_name = group_name if group_name is not None else 'fitting_context_testgroup'
    group = WorkspaceGroup()
    if size is not None:
        for i in range(size):
            ws_name = '{}_{}'.format(group_name, i)
            fake_ws = create_test_workspace(ws_name)
            group.addWorkspace(fake_ws)
    elif items is not None:
        for item in items:
            group.addWorkspace(item)

    ads = AnalysisDataService.Instance()
    ads.addOrReplace(group_name, group)
    return group


def create_test_fit_parameters(test_parameters, global_parameters=None):
    # needs to look like a standard fit table
    fit_table = [{
        'Name': name,
        'Value': value,
        'Error': error
    } for name, (value, error) in test_parameters.items()]

    parameter_workspace = mock.MagicMock()
    parameter_workspace.workspace.__iter__.return_value = fit_table
    return FitParameters(parameter_workspace, global_parameters)


@start_qapplication
class FittingContextTest(unittest.TestCase):
    def setUp(self):
        self.fitting_context = FittingContext()

        self.mock_active_fit_history = mock.PropertyMock(return_value=[])
        type(self.fitting_context).active_fit_history = self.mock_active_fit_history

    def test_items_can_be_added_to_fitting_context(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                'MuonGuassOsc',
                                                mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock())

        self.fitting_context.all_latest_fits = mock.MagicMock(return_value=[fit_information_object])

        self.fitting_context.add_fit(fit_information_object)

        self.assertEqual(fit_information_object,
                         self.fitting_context.all_latest_fits()[0])

    def test_fitfunctions_gives_list_of_unique_function_names(self):
        test_fit_function = 'MuonGuassOsc'

        self.fitting_context.all_latest_fits = mock.MagicMock(return_value=[FitInformation(mock.MagicMock(),
                                                                                           test_fit_function,
                                                                                           mock.MagicMock(),
                                                                                           mock.MagicMock(),
                                                                                           mock.MagicMock())])

        self.fitting_context.add_fit_from_values(mock.MagicMock(),
                                                 test_fit_function,
                                                 mock.MagicMock(),
                                                 mock.MagicMock(),
                                                 mock.MagicMock())
        self.fitting_context.add_fit_from_values(mock.MagicMock(),
                                                 test_fit_function,
                                                 mock.MagicMock(),
                                                 mock.MagicMock(),
                                                 mock.MagicMock())

        fit_functions = self.fitting_context.fit_function_names()

        self.assertEqual(len(fit_functions), 1)
        self.assertEqual(test_fit_function, fit_functions[0])

    def test_can_add_fits_without_first_creating_fit_information_objects(self):
        fit_function_name = 'MuonGuassOsc'
        fit_information_object = FitInformation(mock.MagicMock(), fit_function_name, mock.MagicMock(),
                                                mock.MagicMock(), mock.MagicMock())

        self.fitting_context.all_latest_fits = mock.MagicMock(return_value=[fit_information_object])

        self.fitting_context.add_fit_from_values(mock.MagicMock(), fit_function_name, mock.MagicMock(),
                                                 mock.MagicMock(), mock.MagicMock())

        self.assertEqual(fit_information_object,
                         self.fitting_context.all_latest_fits()[0])

    def test_can_add_fits_with_global_parameters_without_creating_fit_information(
            self):
        parameter_workspace = mock.MagicMock()
        input_workspace = mock.MagicMock()
        fit_function_name = 'MuonGuassOsc'
        global_params = ['A']

        fit_information_object = FitInformation([input_workspace], fit_function_name, mock.MagicMock(),
                                                parameter_workspace, mock.MagicMock(), global_params)

        self.fitting_context.all_latest_fits = mock.MagicMock(return_value=[fit_information_object])

        self.fitting_context.add_fit_from_values([input_workspace], fit_function_name, mock.MagicMock(),
                                                 parameter_workspace, mock.MagicMock(), global_params)

        self.assertEqual(fit_information_object,
                         self.fitting_context.all_latest_fits()[0])

    def test_parameters_are_readonly(self):
        test_parameters = OrderedDict([('Height', (10., 0.4)), ('A0', (1,
                                                                       0.01)),
                                       ('Cost function', (0.1, 0.))])
        fit_params = create_test_fit_parameters(test_parameters)
        fit_info = FitInformation(mock.MagicMock(), mock.MagicMock(),
                                  mock.MagicMock(), fit_params._parameter_workspace, mock.MagicMock())

        self.assertRaises(AttributeError, setattr, fit_info, "parameters",
                          fit_params)

    def test_log_names_returns_logs_from_all_fits_by_default(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))
        fake1 = create_test_workspace(
            ws_name='fake1', time_series_logs=time_series_logs[:2])
        fake2 = create_test_workspace(
            ws_name='fake2', time_series_logs=time_series_logs[2:])
        table_workspace = WorkspaceFactory.createTable()

        fit1 = FitInformation(mock.MagicMock(), 'func1', [StaticWorkspaceWrapper(fake1.name(), fake1)],
                              StaticWorkspaceWrapper(fake1.name(), table_workspace), mock.MagicMock())
        fit2 = FitInformation(mock.MagicMock(), 'func1', [StaticWorkspaceWrapper(fake2.name(), fake2)],
                              StaticWorkspaceWrapper(fake2.name(), table_workspace), mock.MagicMock())

        self.fitting_context.add_fit(fit1)
        self.fitting_context.add_fit(fit2)

        self.mock_active_fit_history = mock.PropertyMock(return_value=[fit1, fit2])
        type(self.fitting_context).active_fit_history = self.mock_active_fit_history
        self.fitting_context.all_latest_fits = mock.MagicMock(return_value=[fit1, fit2])

        log_names = self.fitting_context.log_names()
        self.assertEqual(len(time_series_logs), len(log_names))
        for name, _ in time_series_logs:
            self.assertTrue(
                name in log_names, msg="{} not found in log list".format(name))

    def test_log_names_respects_filter(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))
        fake1 = create_test_workspace(
            ws_name='fake1', time_series_logs=time_series_logs[:2])
        fake2 = create_test_workspace(
            ws_name='fake2', time_series_logs=time_series_logs[2:])

        table_workspace = WorkspaceFactory.createTable()
        fit1 = FitInformation(mock.MagicMock(), 'func1', [StaticWorkspaceWrapper(fake1.name(), fake1)],
                              StaticWorkspaceWrapper(fake1.name(), table_workspace), mock.MagicMock())
        fit2 = FitInformation(mock.MagicMock(), 'func1', [StaticWorkspaceWrapper(fake2.name(), fake2)],
                              StaticWorkspaceWrapper(fake2.name(), table_workspace), mock.MagicMock())

        self.mock_active_fit_history = mock.PropertyMock(return_value=[fit1, fit2])
        type(self.fitting_context).active_fit_history = self.mock_active_fit_history
        self.fitting_context.all_latest_fits = mock.MagicMock(return_value=[fit1, fit2])

        self.fitting_context.add_fit(fit1)
        self.fitting_context.add_fit(fit2)

        required_logs = ('ts_2', 'ts_4')
        log_names = self.fitting_context.log_names(
            filter_fn=lambda log: log.name in required_logs)
        self.assertEqual(len(required_logs), len(log_names))
        for name in required_logs:
            self.assertTrue(
                name in log_names, msg="{} not found in log list".format(name))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
