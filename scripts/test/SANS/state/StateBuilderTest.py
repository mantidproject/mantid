# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from sans.state.IStateParser import IStateParser
from sans.state.StateRunDataBuilder import StateRunDataBuilder
from sans.state.StateBuilder import StateBuilder


class StateBuilderTest(unittest.TestCase):
    def setUp(self):
        self.file_parser = mock.create_autospec(spec=IStateParser)
        self.data_parser = mock.create_autospec(spec=StateRunDataBuilder)
        self.instance = StateBuilder(run_data_builder=self.data_parser, i_state_parser=self.file_parser)

    def test_builder_forwards_from_user_file_parser(self):
        # These should be without modification
        unmodified_methods = {"get_all_states",
                              "get_state_adjustment",
                              "get_state_calculate_transmission",
                              "get_state_compatibility",
                              "get_state_convert_to_q",
                              "get_state_data",
                              "get_state_mask",
                              "get_state_move",
                              "get_state_normalize_to_monitor",
                              "get_state_reduction_mode",
                              "get_state_save",
                              "get_state_scale",
                              "get_state_slice_event",
                              "get_state_wavelength",
                              "get_state_wavelength_and_pixel_adjustment"}

        for method_name in unmodified_methods:
            expected_obj = mock.Mock()
            getattr(self.file_parser, method_name).return_value = expected_obj
            # This should forward on without change
            returned_obj = getattr(self.instance, method_name)()

            getattr(self.file_parser, method_name).assert_called_once()
            self.assertEqual(returned_obj, expected_obj)

    def test_builder_injects_modifications(self):
        modified_methods = {"get_all_states" : "pack_all_states",
                            "get_state_scale": "pack_state_scale"}

        for method_name, data_packing_method in modified_methods.items():
            injected_obj = mock.Mock()  # This should be forwarded to the data builder
            getattr(self.file_parser, method_name).return_value = injected_obj

            # This should forward on to the data_builder instance
            getattr(self.instance, method_name)()

            getattr(self.file_parser, method_name).assert_called_once()
            getattr(self.data_parser, data_packing_method).assert_called_with(injected_obj)


if __name__ == '__main__':
    unittest.main()
