# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from sans.common.enums import SANSInstrument, ReductionMode
from mantidqtinterfaces.sans_isis.gui_logic.gui_common import (
    get_reduction_mode_strings_for_gui,
    get_reduction_selection,
    get_string_for_gui_from_reduction_mode,
    get_batch_file_dir_from_path,
    add_dir_to_datasearch,
    remove_dir_from_datasearch,
    SANSGuiPropertiesHandler,
    get_reduction_mode_from_gui_selection,
)


class GuiCommonTest(unittest.TestCase):
    def _assert_same(self, collection1, collection2):
        for element1, element2 in zip(collection1, collection2):
            self.assertEqual(element1, element2)

    def _assert_same_map(self, map1, map2):
        self.assertEqual(len(map1), len(map2))

        for key, value in map1.items():
            self.assertTrue(key in map2)
            self.assertEqual(map1[key], map2[key])

    def run_reduction_mode_string_case(self, instrument, reduction_mode, reduction_mode_string):
        setting = get_string_for_gui_from_reduction_mode(reduction_mode, instrument)
        self.assertEqual(setting, reduction_mode_string)

    def test_that_gets_reduction_mode_string_for_gui(self):
        sans_settings = get_reduction_mode_strings_for_gui(SANSInstrument.SANS2D)
        self._assert_same(sans_settings, ["rear", "front", "Merged", "All"])

        loq_settings = get_reduction_mode_strings_for_gui(SANSInstrument.LOQ)
        self._assert_same(loq_settings, ["main-detector", "Hab", "Merged", "All"])

        larmor_settings = get_reduction_mode_strings_for_gui(SANSInstrument.LARMOR)
        self._assert_same(larmor_settings, ["DetectorBench"])

        default_settings = get_reduction_mode_strings_for_gui(SANSInstrument.NO_INSTRUMENT)
        self._assert_same(default_settings, ["LAB", "HAB", "Merged", "All"])

    def test_that_gets_correct_reduction_selection(self):
        sans_settings = get_reduction_selection(SANSInstrument.SANS2D)
        self._assert_same_map(
            sans_settings,
            {ReductionMode.LAB: "rear", ReductionMode.HAB: "front", ReductionMode.MERGED: "Merged", ReductionMode.ALL: "All"},
        )

        loq_settings = get_reduction_selection(SANSInstrument.LOQ)
        self._assert_same_map(
            loq_settings,
            {ReductionMode.LAB: "main-detector", ReductionMode.HAB: "Hab", ReductionMode.MERGED: "Merged", ReductionMode.ALL: "All"},
        )

        larmor_settings = get_reduction_selection(SANSInstrument.LARMOR)
        self._assert_same_map(larmor_settings, {ReductionMode.LAB: "DetectorBench"})

        default_settings = get_reduction_selection(SANSInstrument.NO_INSTRUMENT)
        self._assert_same_map(
            default_settings,
            {ReductionMode.LAB: "LAB", ReductionMode.HAB: "HAB", ReductionMode.MERGED: "Merged", ReductionMode.ALL: "All"},
        )

    def test_that_can_get_reduction_mode_string(self):
        self.run_reduction_mode_string_case(SANSInstrument.SANS2D, ReductionMode.LAB, "rear")
        self.run_reduction_mode_string_case(SANSInstrument.LOQ, ReductionMode.HAB, "Hab")
        self.run_reduction_mode_string_case(SANSInstrument.LARMOR, ReductionMode.LAB, "DetectorBench")
        self.run_reduction_mode_string_case(SANSInstrument.NO_INSTRUMENT, ReductionMode.LAB, "LAB")
        self.run_reduction_mode_string_case(SANSInstrument.NO_INSTRUMENT, ReductionMode.HAB, "HAB")

    def test_get_reduction_mode_from_gui_selection(self):
        # Terminology for SANS2D / LOQ / Larmor / Zoom / generic respectively:
        lab_strings = ["rear", "main-detector", "DetectorBench", "rear-detector", "LAB"]
        hab_strings = ["front", "Hab"]
        merged_strings = ["Merged"]
        all_strings = ["All"]

        def check_all_match(str_list, expected_outcome):
            for input_str in str_list:
                self.assertEqual(expected_outcome, get_reduction_mode_from_gui_selection(input_str))
                # Check it's not case sensitive
                input_str = input_str.upper()
                self.assertEqual(expected_outcome, get_reduction_mode_from_gui_selection(input_str))

        check_all_match(lab_strings, ReductionMode.LAB)
        check_all_match(hab_strings, ReductionMode.HAB)
        check_all_match(merged_strings, ReductionMode.MERGED)
        check_all_match(all_strings, ReductionMode.ALL)

    def test_get_reduction_mode_ignores_blank_str(self):
        for blank_str in ["", "   ", "\t"]:
            self.assertEqual(ReductionMode.NOT_SET, get_reduction_mode_from_gui_selection(blank_str))

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


if __name__ == "__main__":
    unittest.main()
