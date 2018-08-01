from __future__ import (absolute_import, division, print_function)

import mantid.api
import tempfile
import os
import random
import string
import unittest
import warnings

from isis_powder.routines import run_details


class ISISPowderInstrumentRunDetailsTest(unittest.TestCase):
    def setup_mock_inst_settings(self, yaml_file_path):
        calibration_dir = tempfile.mkdtemp()
        # Keep track of list of folders to remove
        self._folders_to_remove = [calibration_dir]

        # Check the required unit test files could be found
        test_configuration_path = mantid.api.FileFinder.getFullPath(yaml_file_path)
        if not test_configuration_path or len(test_configuration_path) <= 0:
            self.fail("Could not find the unit test input file called: " + str(yaml_file_path))

        return MockInstSettings(cal_file_path=test_configuration_path, calibration_dir=calibration_dir)

    def tearDown(self):
        for folder in self._folders_to_remove:
            try:
                os.rmdir(folder)
            except OSError:
                warnings.warn("Could not remove the folder at the following path:\n" + str(folder))

    def test_create_run_details_object(self):
        # These attributes are based on a flat YAML file at the specified path
        expected_label = "16_4"
        expected_vanadium_runs = "11-12"
        expected_empty_runs = "13-14"
        expected_offset_file_name = "offset_file_name"
        run_number_string = "17-18"
        mock_inst = self.setup_mock_inst_settings(yaml_file_path="ISISPowderRunDetailsTest.yaml")

        output_obj = run_details.create_run_details_object(run_number_string=run_number_string, inst_settings=mock_inst,
                                                           is_vanadium_run=False)

        self.assertEqual(output_obj.empty_runs, expected_empty_runs)
        self.assertEqual(output_obj.grouping_file_path,
                         os.path.join(mock_inst.calibration_dir, mock_inst.grouping_file_name))
        expected_file_ext = mock_inst.file_extension
        expected_file_ext = expected_file_ext if expected_file_ext.startswith('.') else '.' + expected_file_ext
        self.assertEqual(output_obj.file_extension, expected_file_ext)
        self.assertEqual(output_obj.label, expected_label)
        self.assertEqual(output_obj.offset_file_path,
                         os.path.join(mock_inst.calibration_dir, expected_label, expected_offset_file_name))
        self.assertEqual(output_obj.output_run_string, run_number_string)
        self.assertEqual(output_obj.run_number, 17)
        self.assertEqual(output_obj.vanadium_run_numbers, expected_vanadium_runs)

    def test_create_run_details_object_when_van_cal(self):
        # When we are running the vanadium calibration we expected the run number to take the vanadium
        # number instead
        run_number_string = "17-18"
        expected_vanadium_runs = "11-12"
        mock_inst = self.setup_mock_inst_settings(yaml_file_path="ISISPowderRunDetailsTest.yaml")
        output_obj = run_details.create_run_details_object(run_number_string=run_number_string, inst_settings=mock_inst,
                                                           is_vanadium_run=True)

        self.assertEqual(expected_vanadium_runs, output_obj.run_number)
        self.assertEqual(output_obj.vanadium_run_numbers, output_obj.run_number)
        self.assertEqual(expected_vanadium_runs, output_obj.output_run_string)

    def test_callable_params_are_used(self):
        # These attributes are based on a custom YAML file at the specified path
        expected_label = "16_4"
        expected_vanadium_runs = "11-12"
        expected_empty_runs = "13-14"
        expected_grouping_file_name = "grouping_file"
        run_number_string = "17-18"
        mock_inst = self.setup_mock_inst_settings(yaml_file_path="ISISPowderRunDetailsTestCallable.yaml")

        # Get the YAML file as a dict first
        wrapped_funcs = run_details.RunDetailsWrappedCommonFuncs

        yaml_callable = run_details.CustomFuncForRunDetails(user_function=wrapped_funcs.get_cal_mapping_dict,
                                                            run_number_string=run_number_string,
                                                            inst_settings=mock_inst)

        empty_callable = yaml_callable.add_to_func_chain(user_function=wrapped_funcs.cal_dictionary_key_helper,
                                                         key="custom_empty_run_numbers")
        vanadium_callable = yaml_callable.add_to_func_chain(user_function=wrapped_funcs.cal_dictionary_key_helper,
                                                            key="custom_vanadium_run_numbers")
        grouping_callable = run_details.CustomFuncForRunDetails(user_function=lambda: expected_grouping_file_name)
        output_obj = run_details.create_run_details_object(run_number_string=run_number_string, inst_settings=mock_inst,
                                                           is_vanadium_run=True, empty_run_call=empty_callable,
                                                           vanadium_run_call=vanadium_callable,
                                                           grouping_file_name_call=grouping_callable)

        self.assertEqual(output_obj.label, expected_label)
        self.assertEqual(output_obj.empty_runs, expected_empty_runs)
        self.assertEqual(output_obj.grouping_file_path,
                         os.path.join(mock_inst.calibration_dir, expected_grouping_file_name))
        self.assertEqual(output_obj.vanadium_run_numbers, expected_vanadium_runs)
        self.assertEqual(output_obj.run_number, expected_vanadium_runs)

    def test_run_details_splined_name_list_is_used(self):
        expected_vanadium_runs = "11-12"
        expected_offset_file_name = "offset_file_name"
        splined_name_list = ["bar", "bang", "baz"]
        mock_inst = self.setup_mock_inst_settings(yaml_file_path="ISISPowderRunDetailsTest.yaml")
        output_obj = run_details.create_run_details_object(run_number_string=10, inst_settings=mock_inst,
                                                           is_vanadium_run=False, splined_name_list=splined_name_list)

        expected_splined_out_str = ''.join('_' + val for val in splined_name_list)
        expected_output_name = "VanSplined_" + expected_vanadium_runs + expected_splined_out_str
        expected_output_name += '_' + expected_offset_file_name + ".nxs"
        expected_path = os.path.join(mock_inst.calibration_dir, output_obj.label, expected_output_name)
        self.assertEqual(expected_path, output_obj.splined_vanadium_file_path)


class MockInstSettings(object):
    def __init__(self, cal_file_path, calibration_dir):
        self.calibration_dir = calibration_dir
        self.cal_mapping_path = cal_file_path
        self.grouping_file_name = MockInstSettings.gen_random_string()
        self.file_extension = MockInstSettings.gen_random_string()

    @staticmethod
    def gen_random_string():
        return ''.join(random.choice(string.ascii_lowercase) for _ in range(10))


if __name__ == "__main__":
    unittest.main()
