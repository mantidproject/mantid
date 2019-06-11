# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

from collections import OrderedDict
import unittest

from mantid.api import AnalysisDataService, WorkspaceFactory, WorkspaceGroup
from mantid.kernel import FloatTimeSeriesProperty, StringPropertyWithValue
from mantid.py3compat import iteritems, mock

from Muon.GUI.Common.contexts.fitting_context import FittingContext, FitInformation, FitParameters


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
            for value in values:
                tsp.addValue("2000-05-01T12:00:00", value)
            run.addProperty(name, tsp, replace=True)
    if string_value_logs is not None:
        for name, value in string_value_logs:
            run.addProperty(StringPropertyWithValue(name, value), replace=True)

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
    } for name, (value, error) in iteritems(test_parameters)]

    parameter_workspace = mock.MagicMock()
    parameter_workspace.workspace.__iter__.return_value = fit_table
    return FitParameters(parameter_workspace, global_parameters)


class FitParametersTest(unittest.TestCase):
    def test_equality_with_no_globals(self):
        parameter_workspace = mock.MagicMock()
        fit_params1 = FitParameters(parameter_workspace)
        fit_params2 = FitParameters(parameter_workspace)

        self.assertEqual(fit_params1, fit_params2)

    def test_inequality_with_no_globals(self):
        fit_params1 = FitParameters(mock.MagicMock())
        fit_params2 = FitParameters(mock.MagicMock())

        self.assertNotEqual(fit_params1, fit_params2)

    def test_equality_with_globals(self):
        parameter_workspace = mock.MagicMock()
        fit_params1 = FitParameters(parameter_workspace, ['A'])
        parameter_workspace = parameter_workspace
        fit_params2 = FitParameters(parameter_workspace, ['A'])

        self.assertEqual(fit_params1, fit_params2)

    def test_inequality_with_globals(self):
        parameter_workspace = mock.MagicMock()
        fit_params1 = FitParameters(parameter_workspace, ['A'])
        fit_params2 = FitParameters(parameter_workspace, ['B'])

        self.assertNotEqual(fit_params1, fit_params2)

    def test_length_returns_all_params_with_no_globals(self):
        test_parameters = OrderedDict([('Height', (10., 0.4)), ('A0', (1,
                                                                       0.01)),
                                       ('Cost function', (0.1, 0.))])
        fit_params = create_test_fit_parameters(test_parameters)

        self.assertEqual(3, len(fit_params))

    def test_length_returns_unique_params_with_globals(self):
        test_parameters = OrderedDict([('f0.Height', (10., 0.4)),
                                       ('f0.A0', (1, 0.01)),
                                       ('f1.Height', (10., 0.4)),
                                       ('f1.A0', (2, 0.001)),
                                       ('Cost function', (0.1, 0.))])
        fit_params = create_test_fit_parameters(
            test_parameters, global_parameters=['Height'])

        self.assertEqual(4, len(fit_params))

    def test_names_value_error_returns_all_expected_values_with_no_globals(
            self):
        test_parameters = OrderedDict([('f0.Height', (10., 0.4)),
                                       ('f0.A0', (1, 0.01)),
                                       ('Cost function', (0.1, 0.))])
        fit_params = create_test_fit_parameters(test_parameters)

        self.assertEqual(list(test_parameters.keys()), fit_params.names())
        self.assertEqual(3, len(fit_params))
        for index, name in enumerate(fit_params.names()):
            self.assertEqual(
                test_parameters[name][0],
                fit_params.value(name),
                msg="Mismatch in error for parameter" + name)
            self.assertEqual(
                test_parameters[name][1],
                fit_params.error(name),
                msg="Mismatch in error for parameter" + name)

    def test_names_return_globals_first_with_simultaneous_prefixes_stripped_for_single_fn(
            self):
        # Make some parameters that look like a simultaneous fit of 2 data sets
        test_parameters = OrderedDict([
            ('f0.Height', (10., 0.4)),
            ('f0.A0', (1, 0.01)),  # global
            ('f0.Sigma', (0.01, 0.0001)),  # global
            ('f1.Height', (11., 0.5)),
            ('f1.A0', (1, 0.01)),  # global
            ('f1.Sigma', (0.01, 0.0001)),  # global
            ('Cost function', (0.1, 0.)),
        ])
        global_parameters = ['A0', 'Sigma']
        fit_params = create_test_fit_parameters(test_parameters,
                                                global_parameters)

        expected_keys = [
            'A0', 'Sigma', 'f0.Height', 'f1.Height', 'Cost function'
        ]
        self.assertEqual(expected_keys, fit_params.names())

    def test_names_return_globals_first_with_simultaneous_prefixes_stripped_for_composite_fn(
            self):
        # Make some parameters that look like a simultaneous fit of 2 data sets where parameters
        # could be called the same thing in each function. The values are irrelevant for this test
        test_parameters = OrderedDict([
            # data set 0
            ('f0.f0.A0', (10., 0.4)),
            ('f0.f0.A1', (10., 0.4)),
            ('f0.f1.A0', (10., 0.4)),
            ('f0.f1.A1', (10., 0.4)),
            # data set 1
            ('f1.f0.A0', (10., 0.4)),
            ('f1.f0.A1', (10., 0.4)),
            ('f1.f1.A0', (10., 0.4)),
            ('f1.f1.A1', (10., 0.4)),
            ('Cost function', (0.1, 0.)),
        ])
        global_parameters = ['f0.A0']
        fit_params = create_test_fit_parameters(test_parameters,
                                                global_parameters)

        expected_keys = [
            'f0.A0', 'f0.f0.A1', 'f0.f1.A0', 'f0.f1.A1', 'f1.f0.A1',
            'f1.f1.A0', 'f1.f1.A1', 'Cost function'
        ]
        self.assertEqual(expected_keys, fit_params.names())

    def test_names_value_error_returns_all_expected_values_with_globals(self):
        test_parameters = OrderedDict([
            ('f0.Height', (10., 0.4)),  # global
            ('f0.A0', (1, 0.01)),
            ('f1.Height', (10., 0.4)),  # global
            ('f1.A0', (2, 0.05)),
            ('Cost function', (0.1, 0.)),
        ])
        global_parameters = ['Height']
        # Make some parameters that look like a simultaneous fit
        fit_params = create_test_fit_parameters(test_parameters,
                                                global_parameters)

        expected_keys = ['Height', 'f0.A0', 'f1.A0', 'Cost function']
        self.assertEqual(expected_keys, fit_params.names())
        for index, name in enumerate(fit_params.names()):
            if name == 'Height':
                orig_name = 'f0.Height'
            else:
                orig_name = name
            self.assertEqual(
                test_parameters[orig_name][0],
                fit_params.value(name),
                msg="Mismatch in error for parameter" + name)
            self.assertEqual(
                test_parameters[orig_name][1],
                fit_params.error(name),
                msg="Mismatch in error for parameter" + name)


class FitInformationTest(unittest.TestCase):
    def setUp(self):
        self.fitting_context = FittingContext()

    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_context_constructor_accepts_fit_list(self):
        fit_list = [
            FitInformation(mock.MagicMock(), 'MuonGuassOsc', mock.MagicMock(), mock.MagicMock())
        ]
        context = FittingContext(fit_list)
        self.assertEqual(fit_list, context.fit_list)

    def test_len_gives_length_of_fit_list(self):
        self.assertEqual(0, len(self.fitting_context))
        self.fitting_context.add_fit(
            FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                           mock.MagicMock(), mock.MagicMock()))
        self.assertEqual(1, len(self.fitting_context))

    def test_equality_with_no_globals(self):
        fit_info = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                  mock.MagicMock(), mock.MagicMock())
        self.assertEqual(fit_info, fit_info)

    def test__equality_with_globals(self):
        fit_info = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                  mock.MagicMock(), ['A'])
        self.assertEqual(fit_info, fit_info)

    def test_inequality_with_globals(self):
        fit_info1 = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                   mock.MagicMock(), ['A'])
        fit_info2 = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                   mock.MagicMock(), ['B'])
        self.assertNotEqual(fit_info1, fit_info2)

    def test_items_can_be_added_to_fitting_context(self):
        fit_information_object = FitInformation(
            mock.MagicMock(), 'MuonGuassOsc', mock.MagicMock(), mock.MagicMock())

        self.fitting_context.add_fit(fit_information_object)

        self.assertEqual(fit_information_object,
                         self.fitting_context.fit_list[0])

    def test_empty_global_parameters_if_none_specified(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock())

        self.assertEqual([],
                         fit_information_object.parameters.global_parameters)

    def test_global_parameters_are_captured(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock(),
                                                ['A'])

        self.assertEqual(['A'],
                         fit_information_object.parameters.global_parameters)

    def test_parameters_are_readonly(self):
        test_parameters = OrderedDict([('Height', (10., 0.4)),
                                       ('A0', (1, 0.01)),
                                       ('Cost function', (0.1, 0.))])
        fit_params = create_test_fit_parameters(test_parameters)
        fit_info = FitInformation(fit_params._parameter_workspace,
                                  mock.MagicMock(), mock.MagicMock())

        self.assertRaises(AttributeError, setattr, fit_info, "parameters",
                          fit_params)

    def test_logs_from_workspace_without_logs_returns_emtpy_list(self):
        fake_ws = create_test_workspace()
        fit = FitInformation(mock.MagicMock(), 'func1', fake_ws.name())

        allowed_logs = fit.log_names()
        self.assertEqual(0, len(allowed_logs))

    def test_logs_for_single_workspace_return_all_time_series_logs(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )))
        single_value_logs = (('sv_1', 'val1'), ('sv_2', 'val2'))
        fake_ws = create_test_workspace(time_series_logs=time_series_logs)
        fit = FitInformation(mock.MagicMock(), 'func1', fake_ws.name())

        log_names = fit.log_names()
        for name, _ in time_series_logs:
            self.assertTrue(name in log_names,
                            msg="{} not found in log list".format(name))
        for name, _ in single_value_logs:
            self.assertFalse(name in log_names,
                             msg="{} found in log list".format(name))

    def test_log_names_from_list_of_workspaces_gives_combined_set(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))

        fake1 = create_test_workspace(ws_name='fake1',
                                      time_series_logs=time_series_logs[:2])
        fake2 = create_test_workspace(ws_name='fake2',
                                      time_series_logs=time_series_logs[2:])
        fit = FitInformation(mock.MagicMock(), 'func1',
                             [fake1.name(), fake2.name()])

        log_names = fit.log_names()
        self.assertEqual(len(time_series_logs), len(log_names))
        for name, _ in time_series_logs:
            self.assertTrue(name in log_names,
                            msg="{} not found in log list".format(name))

    def test_log_names_uses_filter_fn(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))
        fake1 = create_test_workspace(ws_name='fake1',
                                      time_series_logs=time_series_logs)
        fit = FitInformation(mock.MagicMock(), 'func1', fake1.name())

        log_names = fit.log_names(lambda log: log.name == 'ts_1')
        self.assertEqual(1, len(log_names))
        self.assertEqual(time_series_logs[0][0], log_names[0])


class FittingContextTest(unittest.TestCase):
    def setUp(self):
        self.fitting_context = FittingContext()

    def test_context_constructor_accepts_fit_list(self):
        fit_list = [
            FitInformation(mock.MagicMock(), 'MuonGuassOsc', mock.MagicMock())
        ]
        context = FittingContext(fit_list)

        self.assertEqual(fit_list, context.fit_list)

    def test_len_gives_length_of_fit_list(self):
        self.assertEqual(0, len(self.fitting_context))
        self.fitting_context.add_fit(
            FitInformation(mock.MagicMock(), 'MuonGuassOsc', mock.MagicMock()))
        self.assertEqual(1, len(self.fitting_context))

    def test_items_can_be_added_to_fitting_context(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                'MuonGuassOsc',
                                                mock.MagicMock())

        self.fitting_context.add_fit(fit_information_object)

        self.assertEqual(fit_information_object,
                         self.fitting_context.fit_list[0])

    def test_fitfunctions_gives_list_of_unique_function_names(self):
        test_fit_function = 'MuonGuassOsc'
        self.fitting_context.add_fit_from_values(mock.MagicMock(),
                                                 test_fit_function,
                                                 mock.MagicMock(), [])
        self.fitting_context.add_fit_from_values(mock.MagicMock(),
                                                 test_fit_function,
                                                 mock.MagicMock(), [])

        fit_functions = self.fitting_context.fit_function_names()

        self.assertEqual(len(fit_functions), 1)
        self.assertEqual(test_fit_function, fit_functions[0])

    def test_can_add_fits_without_first_creating_fit_information_objects(self):
        parameter_workspace = mock.MagicMock()
        input_workspace = mock.MagicMock()
        output_workspace_names = mock.MagicMock()
        fit_function_name = 'MuonGuassOsc'
        fit_information_object = FitInformation(
            parameter_workspace, fit_function_name, input_workspace, output_workspace_names)

        self.fitting_context.add_fit_from_values(
            parameter_workspace, fit_function_name, input_workspace, output_workspace_names)

        self.assertEqual(fit_information_object,
                         self.fitting_context.fit_list[0])

    def test_can_add_fits_with_global_parameters_without_creating_fit_information(
            self):
        parameter_workspace = mock.MagicMock()
        input_workspace = mock.MagicMock()
        fit_function_name = 'MuonGuassOsc'
        global_params = ['A']
        fit_information_object = FitInformation(parameter_workspace,
                                                fit_function_name,
                                                input_workspace, global_params)

        self.fitting_context.add_fit_from_values(
            parameter_workspace, fit_function_name, input_workspace,
            global_params)

        self.assertEqual(fit_information_object,
                         self.fitting_context.fit_list[0])

    def test_parameters_are_readonly(self):
        test_parameters = OrderedDict([('Height', (10., 0.4)),
                                       ('A0', (1, 0.01)),
                                       ('Cost function', (0.1, 0.))])
        fit_params = create_test_fit_parameters(test_parameters)
        fit_info = FitInformation(fit_params._parameter_workspace,
                                  mock.MagicMock(), mock.MagicMock(),
                                  mock.MagicMock())

        self.assertRaises(AttributeError, setattr, fit_info, "parameters",
                          fit_params)

    def test_log_names_returns_logs_from_all_fits_by_default(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))
        fake1 = create_test_workspace(ws_name='fake1',
                                      time_series_logs=time_series_logs[:2])
        fake2 = create_test_workspace(ws_name='fake2',
                                      time_series_logs=time_series_logs[2:])
        self.fitting_context.add_fit(
            FitInformation(mock.MagicMock(), 'func1', fake1.name()))
        self.fitting_context.add_fit(
            FitInformation(mock.MagicMock(), 'func1', fake2.name()))

        log_names = self.fitting_context.log_names()
        self.assertEqual(len(time_series_logs), len(log_names))
        for name, _ in time_series_logs:
            self.assertTrue(name in log_names,
                            msg="{} not found in log list".format(name))

    def test_log_names_respects_filter(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))
        fake1 = create_test_workspace(ws_name='fake1',
                                      time_series_logs=time_series_logs[:2])
        fake2 = create_test_workspace(ws_name='fake2',
                                      time_series_logs=time_series_logs[2:])
        self.fitting_context.add_fit(
            FitInformation(mock.MagicMock(), 'func1', fake1.name()))
        self.fitting_context.add_fit(
            FitInformation(mock.MagicMock(), 'func1', fake2.name()))

        required_logs = ('ts_2', 'ts_4')
        log_names = self.fitting_context.log_names(
            filter_fn=lambda log: log.name in required_logs)
        self.assertEqual(len(required_logs), len(log_names))
        for name in required_logs:
            self.assertTrue(name in log_names,
                            msg="{} not found in log list".format(name))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
