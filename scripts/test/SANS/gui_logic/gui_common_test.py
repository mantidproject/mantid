# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from sans.gui_logic.gui_common import (get_reduction_mode_strings_for_gui, get_reduction_selection,
                                       get_string_for_gui_from_reduction_mode,
                                       get_batch_file_dir_from_path,
                                       add_dir_to_datasearch,
                                       remove_dir_from_datasearch,
                                       SANSGuiPropertiesHandler)
from sans.common.enums import (SANSInstrument, ISISReductionMode)


class GuiCommonTest(unittest.TestCase):
    def _assert_same(self, collection1, collection2):
        for element1, element2 in zip(collection1, collection2):
            self.assertEqual(element1,  element2)

    def _assert_same_map(self, map1, map2):
        self.assertEqual(len(map1),  len(map2))

        for key, value in map1.items():
            self.assertTrue(key in map2)
            self.assertEqual(map1[key],  map2[key])

    def do_test_reduction_mode_string(self, instrument, reduction_mode, reduction_mode_string):
        setting = get_string_for_gui_from_reduction_mode(reduction_mode, instrument)
        self.assertEqual(setting,  reduction_mode_string)

    def test_that_gets_reduction_mode_string_for_gui(self):
        sans_settings = get_reduction_mode_strings_for_gui(SANSInstrument.SANS2D)
        self._assert_same(sans_settings, ["rear", "front", "Merged", "All"])

        loq_settings = get_reduction_mode_strings_for_gui(SANSInstrument.LOQ)
        self._assert_same(loq_settings, ["main-detector", "Hab", "Merged", "All"])

        larmor_settings = get_reduction_mode_strings_for_gui(SANSInstrument.LARMOR)
        self._assert_same(larmor_settings, ["DetectorBench"])

        default_settings = get_reduction_mode_strings_for_gui(SANSInstrument.NoInstrument)
        self._assert_same(default_settings, ["LAB", "HAB", "Merged", "All"])

    def test_that_gets_correct_reduction_selection(self):
        sans_settings = get_reduction_selection(SANSInstrument.SANS2D)
        self._assert_same_map(sans_settings, {ISISReductionMode.LAB: "rear", ISISReductionMode.HAB: "front",
                                              ISISReductionMode.Merged: "Merged", ISISReductionMode.All: "All"})

        loq_settings = get_reduction_selection(SANSInstrument.LOQ)
        self._assert_same_map(loq_settings, {ISISReductionMode.LAB: "main-detector", ISISReductionMode.HAB: "Hab",
                                             ISISReductionMode.Merged: "Merged", ISISReductionMode.All: "All"})

        larmor_settings = get_reduction_selection(SANSInstrument.LARMOR)
        self._assert_same_map(larmor_settings, {ISISReductionMode.LAB: "DetectorBench"})

        default_settings = get_reduction_selection(SANSInstrument.NoInstrument)
        self._assert_same_map(default_settings, {ISISReductionMode.LAB: "LAB", ISISReductionMode.HAB: "HAB",
                                                 ISISReductionMode.Merged: "Merged", ISISReductionMode.All: "All"})

    def test_that_can_get_reduction_mode_string(self):
        self.do_test_reduction_mode_string(SANSInstrument.SANS2D, ISISReductionMode.LAB, "rear")
        self.do_test_reduction_mode_string(SANSInstrument.LOQ, ISISReductionMode.HAB, "Hab")
        self.do_test_reduction_mode_string(SANSInstrument.LARMOR, ISISReductionMode.LAB, "DetectorBench")
        self.do_test_reduction_mode_string(SANSInstrument.NoInstrument, ISISReductionMode.LAB, "LAB")
        self.do_test_reduction_mode_string(SANSInstrument.NoInstrument, ISISReductionMode.HAB, "HAB")

    def test_that_batch_file_dir_returns_none_if_no_forwardslash(self):
        a_path = "test_batch_file_path.csv"
        result = get_batch_file_dir_from_path(a_path)
        self.assertEqual("", result, "Expected empty string. Returned value was {}".format(result))

    def test_correct_batch_file_dir_returned(self):
        a_path = "A/Test/Path/batch_file.csv"
        result = get_batch_file_dir_from_path(a_path)

        expected_result = "A/Test/Path/"
        self.assertEqual(result, expected_result)

    def test_datasearch_directories_updated(self):
        current_dirs = "A/Path/"
        batch_file = "A/Path/To/Batch/File/batch_file.csv"
        _, result = add_dir_to_datasearch(batch_file, current_dirs)

        expected_result = "A/Path/;A/Path/To/Batch/File/"
        self.assertEqual(expected_result, result)

    def test_empty_string_not_added_to_datasearch_directories(self):
        current_dirs = "A/Path/"
        batch_file = "batch_file.csv"
        _, result = add_dir_to_datasearch(batch_file, current_dirs)

        expected_result = "A/Path/"
        self.assertEqual(expected_result, result)

    def test_existing_directory_not_added_to_datasearch_directories(self):
        current_dirs = "A/Path/;A/Path/Already/Added/"
        batch_file = "A/Path/Already/Added/batch_file.csv"
        _, result = add_dir_to_datasearch(batch_file, current_dirs)

        expected_result = "A/Path/;A/Path/Already/Added/"
        self.assertEqual(expected_result, result)

    def test_directories_unchanged_when_removing_empty_string(self):
        current_dirs = "A/Path/;Another/Path/"
        file_to_remove = ""
        result = remove_dir_from_datasearch(file_to_remove, current_dirs)

        expected_result = "A/Path/;Another/Path/"
        self.assertEqual(result, expected_result)

    def test_correct_directory_removed(self):
        current_dirs = "A/Path/;Another/Path/;A/Final/Path/"
        file_to_remove = "Another/Path/"
        result = remove_dir_from_datasearch(file_to_remove, current_dirs)

        expected_result = "A/Path/;A/Final/Path/"
        self.assertEqual(expected_result, result)


class SANSGuiPropertiesHandlerTest(unittest.TestCase):
    @staticmethod
    def test_that_default_functions_are_called_on_initialisation():
        with mock.patch.object(SANSGuiPropertiesHandler, "_load_property", lambda x, y, z: "default_value"):
            default_property_setup_mock = mock.Mock()
            default_values_input = {"a_default_property": (default_property_setup_mock, str)}
            SANSGuiPropertiesHandler(default_values_input)

            default_property_setup_mock.assert_called_once_with("default_value")


if __name__ == '__main__':
    unittest.main()


