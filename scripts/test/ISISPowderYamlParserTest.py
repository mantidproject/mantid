from __future__ import (absolute_import, division, print_function)

import mantid
import os
import tempfile
import unittest
import warnings

from isis_powder.routines import yaml_parser


class ISISPowderYamlParserTest(unittest.TestCase):

    def setUp(self):
        self.temp_file_paths = []

    def tearDown(self):
        for path in self.temp_file_paths:
            try:
                os.remove(path)
            except OSError:
                warnings.warn("Failed to remove unit test temp file at the following path:\n" + str(path))

        self.temp_file_paths = []

    def get_temp_file_handle(self):
        file_handle = tempfile.NamedTemporaryFile(delete=False)
        self.temp_file_paths.append(file_handle.name)
        return file_handle

    def test_dictionary_parses_correctly(self):
        expected_value = "bar"
        second_value = "foo"

        # Write in two ranges to check it determines the correct one
        yaml_handle = self.get_temp_file_handle()
        yaml_handle.write("100-200:\n")
        yaml_handle.write("  test_item: '" + expected_value + "'\n")
        yaml_handle.write("201-300:\n")
        yaml_handle.write("  test_item: '" + second_value + "'\n")
        # Close handle so the test can access it
        yaml_handle.close()

        # Check a random value in the mid point
        returned_dict = yaml_parser.get_run_dictionary(run_number_string="150", file_path=yaml_handle.name)
        self.assertEqual(returned_dict["test_item"], expected_value)

        # Check lower bound is respected
        returned_dict = yaml_parser.get_run_dictionary(run_number_string="100", file_path=yaml_handle.name)
        self.assertEqual(returned_dict["test_item"], expected_value, "Lower bound not respected")

        # Check upper bound is respected
        returned_dict = yaml_parser.get_run_dictionary(run_number_string="200", file_path=yaml_handle.name)
        self.assertEqual(returned_dict["test_item"], expected_value, "Upper bound not respected")

        # Check we can handle a range
        returned_dict = yaml_parser.get_run_dictionary(run_number_string="120-130", file_path=yaml_handle.name)
        self.assertEqual(returned_dict["test_item"], expected_value, "Range returned incorrect value")

        # Check the the second dictionary works at lower bounds
        returned_dict = yaml_parser.get_run_dictionary(run_number_string="201", file_path=yaml_handle.name)
        self.assertEqual(returned_dict["test_item"], second_value)

    def test_file_not_found_gives_sane_err(self):
        # Create a file then delete it so we know it cannot exist at that path
        file_handle = tempfile.NamedTemporaryFile(delete=False)
        file_path = file_handle.name
        file_handle.close()

        os.remove(file_path)
        if os.path.exists(file_path):
            self.fail("File exists after deleting cannot continue this test")

        # Check the error message is there
        with self.assertRaisesRegexp(ValueError, "Config file not found at path"):
            yaml_parser.get_run_dictionary(run_number_string="1", file_path=file_path)

    def test_is_run_range_unbounded(self):
        # Check a valid unbounded range is detected
        result = yaml_parser.is_run_range_key_unbounded("10-")
        self.assertTrue(result, "Unbounded range not detected")

        # Check a bounded range isn't incorrectly detected
        result = yaml_parser.is_run_range_key_unbounded("22")
        self.assertFalse(result, "Single run incorrectly detected")

        # Check a range of runs isn't detected incorrectly
        result = yaml_parser.is_run_range_key_unbounded("33-44")
        self.assertFalse(result, "Bounded range incorrectly detected")

        # What about if it ends in a comma syntax (this will throw elsewhere in the script anyway)
        result = yaml_parser.is_run_range_key_unbounded("55-66,")
        self.assertFalse(result, "Invalid ending character detected as an unbounded")

    def test_blank_file_gives_sane_err(self):
        file_handle = self.get_temp_file_handle()
        # Write nothing and close
        file_path = file_handle.name
        file_handle.close()

        with self.assertRaisesRegexp(ValueError, "YAML files appears to be empty at"):
            yaml_parser.get_run_dictionary(run_number_string=1, file_path=file_path)

    def test_run_number_not_found_gives_sane_err(self):
        expected_val = "yamlParserTest"
        file_handle = self.get_temp_file_handle()

        file_handle.write("10-20:\n")
        file_handle.write("  test_key: '" + expected_val + "'\n")
        file_handle.write("21-:\n")
        file_handle.write("  test_key: '" + expected_val + "'\n")
        file_path = file_handle.name
        file_handle.close()

        # Test a value in the middle of 1-10
        with self.assertRaisesRegexp(ValueError, "Run number 5 not recognised in calibration mapping"):
            yaml_parser.get_run_dictionary(run_number_string="5", file_path=file_path)

        # Check on edge of invalid numbers
        with self.assertRaisesRegexp(ValueError, "Run number 9 not recognised in calibration mapping"):
            yaml_parser.get_run_dictionary(run_number_string=9, file_path=file_path)

        # What about a range of numbers
        with self.assertRaisesRegexp(ValueError, "Run number 2 not recognised in calibration mapping"):
            yaml_parser.get_run_dictionary(run_number_string="2-8", file_path=file_path)

        # Check valid number still works
        returned_dict = yaml_parser.get_run_dictionary(run_number_string="10", file_path=file_path)
        self.assertEqual(returned_dict["test_key"], expected_val)


if __name__ == "__main__":
    unittest.main()
