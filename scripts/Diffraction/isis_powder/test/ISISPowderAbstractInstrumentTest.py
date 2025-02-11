# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid

from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines.instrument_settings import InstrumentSettings
from isis_powder.routines.param_map_entry import ParamMapEntry
from isis_powder.routines import common, run_details, yaml_parser

import os
import random
import string
import tempfile
import unittest
from unittest.mock import patch, create_autospec, call
import warnings


def _gen_random_string():
    return "".join(random.choice(string.ascii_lowercase) for _ in range(10))


class _MockInst(AbstractInst):
    _param_map = [
        ParamMapEntry(ext_name="cal_dir", int_name="calibration_dir"),
        ParamMapEntry(ext_name="cal_map", int_name="cal_mapping_path"),
        ParamMapEntry(ext_name="file_ext", int_name="file_extension", optional=True),
        ParamMapEntry(ext_name="group_file", int_name="grouping_file_name"),
        ParamMapEntry(ext_name="out_dir", int_name="output_dir"),
        ParamMapEntry(ext_name="suffix", int_name="suffix", optional=True),
        ParamMapEntry(ext_name="user_name", int_name="user_name"),
        ParamMapEntry(ext_name="nxs_filename", int_name="nxs_filename"),
        ParamMapEntry(ext_name="gss_filename", int_name="gss_filename"),
        ParamMapEntry(ext_name="dat_files_directory", int_name="dat_files_directory"),
        ParamMapEntry(ext_name="tof_xye_filename", int_name="tof_xye_filename"),
        ParamMapEntry(ext_name="dspacing_xye_filename", int_name="dspacing_xye_filename"),
    ]
    _advanced_config = {
        "user_name": "ISISPowderAbstractInstrumentTest",
        "nxs_filename": "{inst}{runno}{suffix}.nxs",
        "gss_filename": "{inst}{runno}{suffix}.gss",
        "dat_files_directory": "dat_files",
        "tof_xye_filename": "{inst}{runno}_{fileext}{suffix}_b{{bankno}}_TOF.dat",
        "dspacing_xye_filename": "{inst}{runno}{fileext}{suffix}_b{{bankno}}_D.dat",
    }
    INST_PREFIX = "MOCK"

    def __init__(self, cal_file_path, **kwargs):
        self._inst_settings = InstrumentSettings(param_map=self._param_map, adv_conf_dict=self._advanced_config, kwargs=kwargs)
        self.cal_mapping_path = cal_file_path

        super(_MockInst, self).__init__(
            user_name=self._inst_settings.user_name,
            calibration_dir=self._inst_settings.calibration_dir,
            output_dir=self._inst_settings.output_dir,
            inst_prefix=self.INST_PREFIX,
        )


class ISISPowderAbstractInstrumentTest(unittest.TestCase):
    _folders_to_remove = set()
    CALIB_FILE_NAME = "ISISPowderRunDetailsTest.yaml"
    GROUPING_FILE_NAME = "hrpd_new_072_01.cal"

    def _create_temp_dir(self):
        temp_dir = tempfile.mkdtemp()
        self._folders_to_remove.add(temp_dir)
        return temp_dir

    def _find_file_or_die(self, name):
        full_path = mantid.api.FileFinder.getFullPath(name)
        if not full_path:
            self.fail('Could not find file "{}"'.format(name))
        return full_path

    def _setup_output_focused_runs_test(self, mock_get_run_details, mock_get_mode, mock_output_ws, mock_keep_unit):
        mock_inst = self._setup_mock_inst(calibration_dir="ignored", output_dir="ignored", yaml_file_path="ISISPowderRunDetailsTest.yaml")
        mock_run_details = create_autospec(run_details._RunDetails)
        runs = ["123", "124"]
        run_string = "-".join(runs)
        mock_run_details.output_string = run_string
        mock_run_details.output_run_string = run_string
        mock_get_run_details.return_value = mock_run_details
        # produce output that allows us to test that run_details.output_run_string provided is correct
        mock_output_ws.side_effect = lambda focused_run, run_details: 2 * [run_details.output_run_string]

        return mock_get_mode, mock_inst, mock_keep_unit, run_string, runs

    def _setup_mock_inst(
        self,
        yaml_file_path,
        calibration_dir,
        output_dir,
        suffix=None,
        nxs_filename=None,
        tof_xye_filename=None,
        file_ext=None,
        dat_files_directory="",
    ):
        calib_file_path = self._find_file_or_die(self.CALIB_FILE_NAME)
        grouping_file_path = self._find_file_or_die(self.GROUPING_FILE_NAME)
        test_configuration_path = mantid.api.FileFinder.getFullPath(yaml_file_path)
        if not test_configuration_path or len(test_configuration_path) <= 0:
            self.fail("Could not find the unit test input file called: " + str(yaml_file_path))
        return _MockInst(
            cal_file_path=test_configuration_path,
            group_file=grouping_file_path,
            cal_dir=calibration_dir,
            out_dir=output_dir,
            cal_map=calib_file_path,
            suffix=suffix,
            nxs_filename=nxs_filename,
            tof_xye_filename=tof_xye_filename,
            file_ext=file_ext,
            dat_files_directory=dat_files_directory,
        )

    def tearDown(self):
        for folder in self._folders_to_remove:
            try:
                os.rmdir(folder)
            except OSError as exc:
                warnings.warn('Could not remove folder at "{}"\nError message:\n{}'.format(folder, exc))

    def test_generate_out_file_paths_standard_inst_prefix(self):
        mock_inst, run_details, out_dir = self._setup_for_generate_out_file_paths(
            nxs_template="{inst}{runno}{suffix}.nxs", tof_xye_template="", suffix=""
        )

        output_paths = mock_inst._generate_out_file_paths(run_details=run_details)
        expected_nxs_filename = os.path.join(out_dir, "16_4", "ISISPowderAbstractInstrumentTest", "MOCK15.nxs")

        self.assertEqual(output_paths["nxs_filename"], expected_nxs_filename)

    def test_generate_out_file_paths_lower_inst_prefix(self):
        mock_inst, run_details, out_dir = self._setup_for_generate_out_file_paths(nxs_template="{instlow}{runno}{suffix}.nxs", suffix="")

        output_paths = mock_inst._generate_out_file_paths(run_details=run_details)
        expected_nxs_filename = os.path.join(out_dir, "16_4", "ISISPowderAbstractInstrumentTest", "mock15.nxs")

        self.assertEqual(output_paths["nxs_filename"], expected_nxs_filename)

    def test_generate_out_file_paths_with_suffix(self):
        mock_inst, run_details, out_dir = self._setup_for_generate_out_file_paths(nxs_template="{inst}{runno}{suffix}.nxs", suffix="_suf")

        output_paths = mock_inst._generate_out_file_paths(run_details=run_details)
        expected_nxs_filename = os.path.join(out_dir, "16_4", "ISISPowderAbstractInstrumentTest", "MOCK15_suf.nxs")

        self.assertEqual(output_paths["nxs_filename"], expected_nxs_filename)

    def test_generate_out_file_paths_including_fileext(self):
        mock_inst, run_details, out_dir = self._setup_for_generate_out_file_paths(
            nxs_template="{fileext}{inst}{runno}{suffix}.nxs", suffix="", file_ext=".s01"
        )

        output_paths = mock_inst._generate_out_file_paths(run_details=run_details)
        expected_nxs_filename = os.path.join(out_dir, "16_4", "ISISPowderAbstractInstrumentTest", "s01MOCK15.nxs")

        self.assertEqual(output_paths["nxs_filename"], expected_nxs_filename)

    def test_generate_out_file_paths_respects_dat_files_directory_where_appropriate(self):
        mock_inst, run_details, out_dir = self._setup_for_generate_out_file_paths(
            nxs_template="{inst}{runno}_{fileext}_1.nxs",
            tof_xye_template="{inst}{runno}_{fileext}{suffix}_b1_TOF.dat",
            suffix="_suf",
            file_ext=".s01",
            dat_files_dir="dat_files",
        )

        output_paths = mock_inst._generate_out_file_paths(run_details=run_details)
        expected_nxs_filename = os.path.join(out_dir, "16_4", "ISISPowderAbstractInstrumentTest", "MOCK15_s01_1.nxs")
        expected_xye_filename = os.path.join(out_dir, "16_4", "ISISPowderAbstractInstrumentTest", "dat_files", "MOCK15_s01_suf_b1_TOF.dat")

        self.assertEqual(output_paths["nxs_filename"], expected_nxs_filename)
        self.assertEqual(output_paths["tof_xye_filename"], expected_xye_filename)

    def _setup_for_generate_out_file_paths(self, nxs_template="", tof_xye_template="", suffix=None, file_ext=None, dat_files_dir=""):
        cal_dir = self._create_temp_dir()
        out_dir = self._create_temp_dir()

        mock_inst = self._setup_mock_inst(
            suffix=suffix,
            file_ext=file_ext,
            yaml_file_path="ISISPowderRunDetailsTest.yaml",
            calibration_dir=cal_dir,
            output_dir=out_dir,
            nxs_filename=nxs_template,
            tof_xye_filename=tof_xye_template,
            dat_files_directory=dat_files_dir,
        )

        run_number = 15
        run_number2 = common.get_first_run_number(run_number_string=run_number)
        cal_mapping_dict = yaml_parser.get_run_dictionary(run_number_string=run_number2, file_path=mock_inst.cal_mapping_path)

        grouping_filename = _gen_random_string()
        empty_runs = common.cal_map_dictionary_key_helper(dictionary=cal_mapping_dict, key="empty_run_numbers")
        vanadium_runs = common.cal_map_dictionary_key_helper(dictionary=cal_mapping_dict, key="vanadium_run_numbers")

        run_details_obj = run_details.create_run_details_object(
            run_number_string=run_number,
            inst_settings=mock_inst._inst_settings,
            is_vanadium_run=False,
            grouping_file_name=grouping_filename,
            empty_inst_run_number=empty_runs,
            vanadium_string=vanadium_runs,
        )

        return mock_inst, run_details_obj, out_dir

    def test_set_valid_beam_parameters(self):
        # Setup basic instrument mock
        cal_dir = self._create_temp_dir()
        out_dir = self._create_temp_dir()
        mock_inst = self._setup_mock_inst(calibration_dir=cal_dir, output_dir=out_dir, yaml_file_path="ISISPowderRunDetailsTest.yaml")

        # Test valid parameters are retained
        mock_inst.set_beam_parameters(height=1.234, width=2)
        self.assertEqual(mock_inst._beam_parameters["height"], 1.234)
        self.assertEqual(mock_inst._beam_parameters["width"], 2)

    def test_set_invalid_beam_parameters(self):
        # Setup basic instrument mock
        cal_dir = self._create_temp_dir()
        out_dir = self._create_temp_dir()
        mock_inst = self._setup_mock_inst(calibration_dir=cal_dir, output_dir=out_dir, yaml_file_path="ISISPowderRunDetailsTest.yaml")

        # Test combination of positive / negative raise exceptions
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=1.234, width=-2)
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=-1.234, width=2)
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=-1.234, width=-2)

        # Test non-numerical input
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height="height", width=-2)
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=-1.234, width=True)

    @patch("isis_powder.routines.common.remove_intermediate_workspace")
    @patch("isis_powder.routines.common.keep_single_ws_unit")
    @patch("isis_powder.abstract_inst.AbstractInst._output_focused_ws")
    @patch("isis_powder.abstract_inst.AbstractInst._get_input_batching_mode")
    @patch("isis_powder.abstract_inst.AbstractInst._get_run_details")
    def test_output_focused_runs_individual_batch_mode(
        self, mock_get_run_details, mock_get_mode, mock_output_ws, mock_keep_unit, mock_remove_ws
    ):
        mock_get_mode, mock_inst, mock_keep_unit, run_string, runs = self._setup_output_focused_runs_test(
            mock_get_run_details, mock_get_mode, mock_output_ws, mock_keep_unit
        )
        mock_get_mode.return_value = "Individual"

        mock_inst._output_focused_runs(focused_runs=["INST123_foc", "INST124_foc"], run_number_string=run_string)

        expected_calls = [call(d_spacing_group=run, tof_group=run, unit_to_keep=None) for run in runs]
        mock_keep_unit.assert_has_calls(expected_calls)

    @patch("isis_powder.routines.common.remove_intermediate_workspace")
    @patch("isis_powder.routines.common.keep_single_ws_unit")
    @patch("isis_powder.abstract_inst.AbstractInst._output_focused_ws")
    @patch("isis_powder.abstract_inst.AbstractInst._get_input_batching_mode")
    @patch("isis_powder.abstract_inst.AbstractInst._get_run_details")
    def test_output_focused_runs_summed_batch_mode(
        self, mock_get_run_details, mock_get_mode, mock_output_ws, mock_keep_unit, mock_remove_ws
    ):
        mock_get_mode, mock_inst, mock_keep_unit, run_string, runs = self._setup_output_focused_runs_test(
            mock_get_run_details, mock_get_mode, mock_output_ws, mock_keep_unit
        )
        mock_get_mode.return_value = "Summed"

        mock_inst._output_focused_runs(focused_runs=["INST123_foc", "INST124_foc"], run_number_string=run_string)

        expected_calls = 2 * [call(d_spacing_group=run_string, tof_group=run_string, unit_to_keep=None)]
        mock_keep_unit.assert_has_calls(expected_calls)


if __name__ == "__main__":
    unittest.main()
