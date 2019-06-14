# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)
import unittest
from mantid.py3compat import mock
from Muon.GUI.Common.contexts.fitting_context import FittingContext, FitInformation


class FittingContextTest(unittest.TestCase):
    def setUp(self):
        self.fitting_context = FittingContext()

    def test_len_gives_length_of_fit_list(self):
        self.assertEqual(0, len(self.fitting_context))
        self.fitting_context.add_fit(
            FitInformation(mock.MagicMock(), 'MuonGuassOsc', mock.MagicMock(), []))
        self.assertEqual(1, len(self.fitting_context))

    def test_items_can_be_added_to_fitting_context(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                'MuonGuassOsc',
                                                mock.MagicMock(),
                                                [])

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

    def test_can_retrieve_a_list_of_fit_objects_based_on_fit_function_name(
            self):
        fit_information_object_0 = FitInformation(mock.MagicMock(),
                                                  'MuonGuassOsc',
                                                  mock.MagicMock(), [])
        fit_information_object_1 = FitInformation(mock.MagicMock(), 'MuonOsc',
                                                  mock.MagicMock(), [])
        fit_information_object_2 = FitInformation(mock.MagicMock(),
                                                  'MuonGuassOsc',
                                                  mock.MagicMock(), [])
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
        fit_information_object = FitInformation(parameter_workspace,
                                                fit_function_name,
                                                input_workspace, [])

        self.fitting_context.add_fit_from_values(parameter_workspace,
                                                 fit_function_name,
                                                 input_workspace, [])

        self.assertEqual(fit_information_object,
                         self.fitting_context.fit_list[0])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
