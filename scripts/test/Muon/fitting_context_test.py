# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

from collections import OrderedDict
import unittest

from mantid.py3compat import iteritems, mock

from Muon.GUI.Common.contexts.fitting_context import FittingContext, FitInformation, FitParameters


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

    def test_fitinformation_equality_with_no_globals(self):
        fit_info = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                  mock.MagicMock())
        self.assertEqual(fit_info, fit_info)

    def test_fitinformation_equality_with_globals(self):
        fit_info = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                  mock.MagicMock(), ['A'])
        self.assertEqual(fit_info, fit_info)

    def test_fitinformation_inequality_with_globals(self):
        fit_info1 = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                   mock.MagicMock(), ['A'])
        fit_info2 = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                   mock.MagicMock(), ['B'])
        self.assertNotEqual(fit_info1, fit_info2)

    def test_items_can_be_added_to_fitting_context(self):
        fit_information_object = FitInformation(
            mock.MagicMock(), 'MuonGuassOsc', mock.MagicMock())

        self.fitting_context.add_fit(fit_information_object)

        self.assertEqual(fit_information_object,
                         self.fitting_context.fit_list[0])

    def test_empty_global_parameters_if_none_specified(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock())

        self.assertEqual([],
                         fit_information_object.parameters.global_parameters)

    def test_global_parameters_are_captured(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock(), ['A'])

        self.assertEqual(['A'],
                         fit_information_object.parameters.global_parameters)

    def test_fitfunctions_gives_list_of_unique_function_names(self):
        test_fit_function = 'MuonGuassOsc'
        self.fitting_context.add_fit_from_values(mock.MagicMock(),
                                                 test_fit_function,
                                                 mock.MagicMock())
        self.fitting_context.add_fit_from_values(mock.MagicMock(),
                                                 test_fit_function,
                                                 mock.MagicMock())

        fit_functions = self.fitting_context.fit_function_names()

        self.assertEqual(len(fit_functions), 1)
        self.assertEqual(test_fit_function, fit_functions[0])

    def test_can_retrieve_a_list_of_fit_objects_based_on_fit_function_name(
            self):
        fit_information_object_0 = FitInformation(mock.MagicMock(),
                                                  'MuonGuassOsc',
                                                  mock.MagicMock())
        fit_information_object_1 = FitInformation(mock.MagicMock(), 'MuonOsc',
                                                  mock.MagicMock())
        fit_information_object_2 = FitInformation(mock.MagicMock(),
                                                  'MuonGuassOsc',
                                                  mock.MagicMock())
        self.fitting_context.add_fit(fit_information_object_0)
        self.fitting_context.add_fit(fit_information_object_1)
        self.fitting_context.add_fit(fit_information_object_2)

        result = self.fitting_context.find_fits_for_function('MuonGuassOsc')

        self.assertEqual([fit_information_object_0, fit_information_object_2],
                         result)

    def test_can_add_fits_without_first_creating_fit_information_objects(self):
        parameter_workspace = mock.MagicMock()
        input_workspace = mock.MagicMock()
        fit_function_name = 'MuonGuassOsc'
        fit_information_object = FitInformation(
            parameter_workspace, fit_function_name, input_workspace)

        self.fitting_context.add_fit_from_values(
            parameter_workspace, fit_function_name, input_workspace)

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
        test_parameters = OrderedDict([('Height', (10., 0.4)), ('A0', (1,
                                                                       0.01)),
                                       ('Cost function', (0.1, 0.))])
        fit_params = create_test_fit_parameters(test_parameters)
        fit_info = FitInformation(fit_params._parameter_workspace,
                                  mock.MagicMock(), mock.MagicMock())

        self.assertRaises(AttributeError, setattr, fit_info, "parameters",
                          fit_params)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
