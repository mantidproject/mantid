# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from sans.gui_logic.sans_data_processor_gui_algorithm import (create_properties, create_option_column_properties,
                                                              get_gui_algorithm_name, get_white_list, get_black_list)
from sans.common.enums import (SANSFacility)


class SANSGuiDataProcessorAlgorithmTest(unittest.TestCase):
    def test_that_all_option_columns_are_there(self):
        options = create_option_column_properties()
        self.assertEqual(len(options), 3)
        self.assertEqual(options[0].algorithm_property, "WavelengthMin")
        self.assertEqual(options[1].algorithm_property, "WavelengthMax")
        self.assertEqual(options[2].algorithm_property, "EventSlices")

    def test_that_the_properties_with_periods_can_be_provided(self):
        props = create_properties()
        self.assertEqual(len(props), 20)

        expected = [{"algorithm_property": "SampleScatter", "column_name": "SampleScatter"},
                    {"algorithm_property": "SampleScatterPeriod", "column_name": "ssp"},
                    {"algorithm_property": "SampleTransmission", "column_name": "SampleTrans"},
                    {"algorithm_property": "SampleTransmissionPeriod", "column_name": "stp"},
                    {"algorithm_property": "SampleDirect", "column_name": "SampleDirect"},
                    {"algorithm_property": "SampleDirectPeriod", "column_name": "sdp"},
                    {"algorithm_property": "CanScatter", "column_name": "CanScatter"},
                    {"algorithm_property": "CanScatterPeriod", "column_name": "csp"},
                    {"algorithm_property": "CanTransmission", "column_name": "CanTrans"},
                    {"algorithm_property": "CanTransmissionPeriod", "column_name": "ctp"},
                    {"algorithm_property": "CanDirect", "column_name": "CanDirect"},
                    {"algorithm_property": "CanDirectPeriod", "column_name": "cdp"},
                    {"algorithm_property": "UseOptimizations", "column_name": ""},
                    {"algorithm_property": "PlotResults", "column_name": ""},
                    {"algorithm_property": "OutputName", "column_name": "OutputName"},
                    {"algorithm_property": "UserFile", "column_name": "User File"},
                    {"algorithm_property": "SampleThickness", "column_name": "Sample Thickness"},
                    {"algorithm_property": "RowIndex", "column_name": ""},
                    {"algorithm_property": "OutputMode", "column_name": ""},
                    {"algorithm_property": "OutputGraph", "column_name": ""}]

        for index, element in enumerate(props):
            self.assertEqual(element.algorithm_property, expected[index]["algorithm_property"])
            self.assertEqual(element.column_name, expected[index]["column_name"])

    def test_that_the_properties_without_periods_can_be_provided(self):
        props = create_properties(show_periods=False)
        self.assertEqual(len(props), 14)

        expected = [{"algorithm_property": "SampleScatter", "column_name": "SampleScatter"},
                    {"algorithm_property": "SampleTransmission", "column_name": "SampleTrans"},
                    {"algorithm_property": "SampleDirect", "column_name": "SampleDirect"},
                    {"algorithm_property": "CanScatter", "column_name": "CanScatter"},
                    {"algorithm_property": "CanTransmission", "column_name": "CanTrans"},
                    {"algorithm_property": "CanDirect", "column_name": "CanDirect"},
                    {"algorithm_property": "UseOptimizations", "column_name": ""},
                    {"algorithm_property": "PlotResults", "column_name": ""},
                    {"algorithm_property": "OutputName", "column_name": "OutputName"},
                    {"algorithm_property": "UserFile", "column_name": "User File"},
                    {"algorithm_property": "SampleThickness", "column_name": "Sample Thickness"},
                    {"algorithm_property": "RowIndex", "column_name": ""},
                    {"algorithm_property": "OutputMode", "column_name": ""},
                    {"algorithm_property": "OutputGraph", "column_name": ""}]

        for index, element in enumerate(props):
            self.assertEqual(element.algorithm_property, expected[index]["algorithm_property"])
            self.assertEqual(element.column_name, expected[index]["column_name"])

    def test_that_gets_gui_algorithm_name(self):
        alg_name = get_gui_algorithm_name(SANSFacility.ISIS)
        self.assertEqual(alg_name, "SANSGuiDataProcessorAlgorithm")

    def test_that_getting_algorithm_name_raises_when_facility_is_not_known(self):
        args = ["teskdfsd"]
        self.assertRaises(RuntimeError, get_gui_algorithm_name, *args)

    def test_that_white_list_contains_all_properties(self):
        white_list = get_white_list()
        self.assertEqual(len(white_list), 20)

    def test_that_black_list_contains_input_and_output_ws(self):
        black_list = get_black_list()
        self.assertTrue(black_list)
        self.assertTrue(black_list.startswith("InputWorkspace,OutputWorkspace"))


if __name__ == '__main__':
    unittest.main()
