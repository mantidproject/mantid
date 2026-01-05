# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
import tempfile
import numpy as np
from unittest.mock import patch, MagicMock, mock_open, call
from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace, SetGoniometer, SetSample
from Engineering.common.xml_shapes import get_cube_xml

from Engineering.texture.correction.correction_model import TextureCorrectionModel

correction_model_path = "Engineering.texture.correction.correction_model"
texture_helper_path = "Engineering.texture.texture_helper"


class TextureCorrectionModelTest(unittest.TestCase):
    def setUp(self):
        self.model = TextureCorrectionModel()
        self.ws_name = "test_ws"

        # Mock instrument
        mock_inst = MagicMock()
        mock_inst.getName.return_value = "instrument"

        # Mock run_number log property
        mock_run_number = MagicMock()
        mock_run_number.value = "123456"

        # Mock run object to return the log
        mock_run = MagicMock()
        mock_run.getProperty.side_effect = lambda key: mock_run_number if key == "run_number" else MagicMock()

        # Mock Sample
        mock_sample = MagicMock()

        # Mock Sample
        mock_material = MagicMock()
        mock_material.name.return_value = "Fe"
        mock_sample.getMaterial.return_value = mock_material

        # Mock Goniometer
        mock_goniometer = MagicMock()
        mock_goniometer.getR.return_value = np.eye(3)

        # Mock workspace
        self.mock_ws = MagicMock()
        self.mock_ws.getInstrument.return_value = mock_inst
        self.mock_ws.getRun.return_value = mock_run
        self.mock_ws.run.return_value = mock_run
        self.mock_ws.sample.return_value = mock_sample

    def tearDown(self):
        if ADS.doesExist(self.ws_name):
            ADS.remove(self.ws_name)

    def run_euler_gonio_test(self, test_wss, txt_data, euler_scheme, euler_sense, target_calls, mock_set_gonio):
        mock_set_gonio.reset_mock()
        with patch("builtins.open", mock_open(read_data=txt_data)):
            self.model.load_all_orientations(test_wss, "angles.txt", use_euler=True, euler_scheme=euler_scheme, euler_sense=euler_sense)

        self.assertEqual(mock_set_gonio.call_count, 2)
        expected_calls = [call(self.mock_ws, **target_call) for target_call in target_calls]
        mock_set_gonio.assert_has_calls(expected_calls)

    @patch(correction_model_path + ".TextureCorrectionModel._has_no_orientation_set")
    @patch(correction_model_path + ".TextureCorrectionModel._has_no_valid_material")
    @patch(correction_model_path + ".TextureCorrectionModel._has_no_valid_shape")
    @patch(correction_model_path + ".ADS")
    def test_get_ws_info_returns_all_fields(self, mock_ads, mock_no_valid_shape, mock_no_valid_material, mock_no_valid_orientation):
        mock_ads.retrieve.return_value = self.mock_ws
        mock_no_valid_shape.return_value = True
        mock_no_valid_material.return_value = True
        mock_no_valid_orientation.return_value = True

        info = self.model.get_ws_info(self.ws_name)

        mock_no_valid_shape.assert_called_once_with(self.ws_name)
        mock_no_valid_material.assert_called_once_with(self.ws_name)
        mock_no_valid_orientation.assert_called_once_with(self.ws_name)

        self.assertIn("shape", info)
        self.assertIn("material", info)
        self.assertIn("orient", info)
        self.assertIn("select", info)

    @patch(correction_model_path + ".TextureCorrectionModel._has_no_orientation_set")
    @patch(correction_model_path + ".TextureCorrectionModel._has_no_valid_material")
    @patch(correction_model_path + ".TextureCorrectionModel._has_no_valid_shape")
    @patch(correction_model_path + ".ADS")
    def test_get_ws_info_returns_a_material_if_present(
        self, mock_ads, mock_no_valid_shape, mock_no_valid_material, mock_no_valid_orientation
    ):
        mock_ads.retrieve.return_value = self.mock_ws
        mock_no_valid_shape.return_value = True
        mock_no_valid_material.return_value = False
        mock_no_valid_orientation.return_value = True

        info = self.model.get_ws_info(self.ws_name)

        mock_no_valid_shape.assert_called_once_with(self.ws_name)
        mock_no_valid_material.assert_called_once_with(self.ws_name)
        mock_no_valid_orientation.assert_called_once_with(self.ws_name)

        self.assertIn("shape", info)
        self.assertIn("material", info)
        self.assertIn("orient", info)
        self.assertIn("select", info)

        self.assertEqual(info["material"], "Fe")

    @patch(correction_model_path + ".ADS")
    def test_get_material_name_returns_default(self, mock_ads):
        mock_ads.retrieve.return_value = self.mock_ws
        name = self.model._get_material_name(self.ws_name)
        self.assertEqual(name, "Fe")

    def test_copy_sample_moves_correct_shape_and_retains_correct_orientation(self):
        # Create a reference ws with a goniometer set and a shape
        CreateSampleWorkspace(OutputWorkspace=self.ws_name)
        SetGoniometer(self.ws_name, Axis0="30,0,1,0,1", Axis1="15,0,0,1,1", Axis2="30,0,1,0,1")
        SetSample(self.ws_name, Geometry={"Shape": "CSG", "Value": get_cube_xml("cube", 10.0)})

        # Create another ws with a different Goniometer set to copy the shape to
        other_ws = "copy_to_ws"
        copy_ws = CreateSampleWorkspace(OutputWorkspace=other_ws)
        SetGoniometer(other_ws, Axis0="20,0,1,0,1", Axis1="35,0,0,1,1", Axis2="10,0,1,0,1")
        copy_mat = copy_ws.getRun().getGoniometer().getR()
        self.model.copy_sample_info(self.ws_name, [other_ws])

        # Create a third ws with the desired shape and goniometer set directly with algs
        non_copy_str = "non_copy"
        uncopy_ws = CreateSampleWorkspace(OutputWorkspace=non_copy_str)
        SetGoniometer(non_copy_str, Axis0="20,0,1,0,1", Axis1="35,0,0,1,1", Axis2="10,0,1,0,1")
        SetSample(non_copy_str, Geometry={"Shape": "CSG", "Value": get_cube_xml("cube", 10.0)})
        ref_shape = uncopy_ws.sample().getShape()
        ref_mat = uncopy_ws.getRun().getGoniometer().getR()

        # compare the second and third ws shapes
        copied_ws = ADS.retrieve(other_ws)
        copied_shape = copied_ws.sample().getShape()
        copied_mat = copied_ws.getRun().getGoniometer().getR()

        # assert shapes and orientations are the same
        self.assertTrue(np.all(ref_shape.getMesh() == copied_shape.getMesh()))
        self.assertTrue(np.all(copied_mat == copy_mat))
        self.assertTrue(np.all(copied_mat == ref_mat))
        ADS.clear()

    def test_has_no_valid_material_returns_true_on_empty(self):
        # Force material to be empty
        CreateSampleWorkspace(OutputWorkspace=self.ws_name)
        self.assertTrue(self.model._has_no_valid_material(self.ws_name))

    def test_param_str_to_dict_converts_simple_values(self):
        param_str = "NumberOfWavelengthPoints:50,NumberOfDetectorRows:10,SparseInstrument:True"
        result = self.model._param_str_to_dict(param_str)
        self.assertEqual(result["NumberOfWavelengthPoints"], 50)
        self.assertEqual(result["NumberOfDetectorRows"], 10)
        self.assertTrue(result["SparseInstrument"])

    @patch(correction_model_path + ".ADS")
    def test_get_atten_table_name_formats_correctly(self, mock_ads):
        mock_ads.retrieve.return_value = self.mock_ws

        # Run the method
        table_name = self.model.get_atten_table_name(self.ws_name, 2.5, "Wavelength")

        # Assert structure
        self.assertEqual("instrument_123456_attenuation_coefficient_2.5_Wavelength", table_name)

    @patch(correction_model_path + ".ConvertUnits")
    @patch(correction_model_path + ".ADS")
    def test_read_attenuation_coefficient_at_value_interpolates(self, mock_ads, mock_convert):
        mock_ads.retrieve.return_value = self.mock_ws
        mock_ws = self.mock_ws
        mock_ws.readX.return_value = np.array([1, 2, 3])
        mock_ws.readY.return_value = np.array([2, 4])
        mock_ws.getNumberHistograms.return_value = 1

        mock_convert.return_value = mock_ws

        coefs = self.model.read_attenuation_coefficient_at_value(self.ws_name, 2.0, "Wavelength")
        self.assertEqual(len(coefs), 1)
        # bin centres are 1.5 and 2.5, so expect a value halfway between 2 and 4
        self.assertEqual(coefs[0], 3)

    def test_validate_file_on_expected_inputs(self):
        valid_file_paths = ["anything.txt", "C:/anything.txt", r"D:\any\thing.txt"]
        ext = ".txt"

        for file in valid_file_paths:
            result = self.model._validate_file(file, ext)
            null_result = self.model._validate_file(file, ".xml")
            self.assertTrue(result)
            self.assertFalse(null_result)

    @patch(texture_helper_path + ".get_cube_xml")
    @patch(correction_model_path + ".DefineGaugeVolume")
    def test_define_gauge_vol_for_preset(self, mock_def_gauge_vol, mock_get_xml):
        # this method under the hood should just invoke get_cube_xml with the preset values for a 4mm gauge volume
        preset = "4mmCube"
        # the actual xml for this is unimportant so we can use a dummy string, what matters is:
        # (1) the method is called with the correct parameters
        # and (2) the return value is then passed to def gauge volume
        dummy_xml = "example dummy xml string"
        mock_get_xml.return_value = dummy_xml
        self.model.define_gauge_volume(self.ws_name, preset=preset, custom=None)

        # check (1)
        mock_get_xml.assert_called_once_with("some-gv", 0.004)
        # check (2)
        mock_def_gauge_vol.assert_called_once_with(self.ws_name, dummy_xml)

    @patch(correction_model_path + ".DefineGaugeVolume")
    @patch(texture_helper_path + "._read_xml")
    def test_define_gauge_vol_for_custom(self, mock_read_xml, mock_def_gauge_vol):
        preset = "Custom Shape"
        expected_str = get_cube_xml("some-gv", 0.004)
        mock_read_xml.return_value = expected_str
        self.model.define_gauge_volume(self.ws_name, preset=preset, custom="gv.xml")
        mock_def_gauge_vol.assert_called_once_with(self.ws_name, expected_str)

    @patch(correction_model_path + ".LoadSampleShape")
    @patch(correction_model_path + ".SetSampleMaterial")
    def test_set_sample_info_with_stl_calls_load_sample_shape(self, mock_set_mat, mock_load_shape):
        self.model.set_sample_info("ws1", "model.stl", "Fe")
        mock_load_shape.assert_called_once()
        mock_set_mat.assert_called_once()

    @patch(correction_model_path + ".SetSample")
    @patch(correction_model_path + ".SetSampleMaterial")
    def test_set_sample_info_with_csg_calls_set_sample(self, mock_set_mat, mock_set_sample):
        self.model.set_sample_info("ws1", "sphere", "Fe")
        mock_set_sample.assert_called_once()
        mock_set_mat.assert_called_once()

    @patch(correction_model_path + ".CloneWorkspace")
    @patch(correction_model_path + ".CopySample")
    @patch(correction_model_path + ".ADS")
    def test_copy_sample_info_creates_temp_workspace_and_copies(self, mock_ads, mock_copy_sample, mock_clone):
        mock_ws = MagicMock()
        mock_ws.getRun().getGoniometer().getR.return_value = np.eye(3)
        mock_ads.retrieve.return_value = mock_ws
        mock_clone.return_value = mock_ws

        self.model.copy_sample_info("ws1", ["ws2", "ws3"])
        self.assertEqual(mock_copy_sample.call_count, 3)

    @patch(correction_model_path + ".ADS")
    def test_has_no_orientation_set_returns_true_on_identity(self, mock_ads):
        ws = MagicMock()
        ws.run().getGoniometer().getR.return_value = np.eye(3)
        mock_ads.retrieve.return_value = ws
        self.assertTrue(self.model._has_no_orientation_set("ws"))

    def test_parse_param_values_parses_types_correctly(self):
        self.assertTrue(self.model._parse_param_values("true"))
        self.assertFalse(self.model._parse_param_values("false"))
        self.assertEqual(self.model._parse_param_values("42"), 42)
        self.assertEqual(self.model._parse_param_values("abc"), "abc")

    @patch(correction_model_path + ".MonteCarloAbsorption")
    @patch(correction_model_path + ".ConvertUnits")
    def test_calc_absorption_executes_expected_chain(self, mock_convert, mock_mc):
        ws = MagicMock()
        mock_convert.return_value = ws
        self.model.calc_absorption("ws1", "NumberOfWavelengthPoints:10")
        mock_mc.assert_called_once()
        self.assertEqual(mock_mc.call_args[1]["OutputWorkspace"], "_abs_corr")

    @patch(correction_model_path + ".TextureCorrectionModel.get_thetas")
    @patch(correction_model_path + ".ADS")
    @patch(correction_model_path + ".CloneWorkspace")
    def test_calc_divergence(self, mock_clone_ws, mock_ads, mock_get_thetas):
        self.mock_ws.getNumberHistograms.return_value = 2
        self.mock_ws.readY.return_value = np.array([1.0, 2.0])
        mock_get_thetas.return_value = np.array([0, 0.5])  # radians
        mock_ads.retrieve.return_value = self.mock_ws

        # Detector table with fixed theta values
        mock_si = MagicMock()
        self.mock_ws.spectrumInfo.return_value = mock_si

        # Mock output_ws
        mock_out_ws = MagicMock()
        mock_clone_ws.return_value = mock_out_ws

        self.model.calc_divergence(self.ws_name, horz=1.0, vert=2.0, det_horz=3.0)

        expected_thetas = [0, 0.5]
        expected_divs = 2.0 * np.sqrt(1.0**2 + 3.0**2) * np.sin(expected_thetas) ** 2

        calls = mock_out_ws.setY.call_args_list
        self.assertEqual(len(calls), 2)

        for i, call_args in enumerate(calls):
            index_arg, array_arg = call_args[0]
            self.assertEqual(index_arg, i)
            np.testing.assert_array_almost_equal(array_arg, np.ones_like(self.mock_ws.readY.return_value) * expected_divs[i])

    @patch(correction_model_path + ".TextureCorrectionModel._save_corrected_files")
    @patch(correction_model_path + ".ConvertUnits")
    @patch(correction_model_path + ".CloneWorkspace")
    @patch(correction_model_path + ".ADS")
    def test_apply_corrections_applies_default_normalisation_with_default_vals(self, mock_ads, mock_clone, mock_convert, mock_save):
        mock_calib = mock.MagicMock()
        self.model.set_calibration(mock_calib)
        mock_ads.retrieve.return_value = self.mock_ws
        mock_convert.return_value = mock.MagicMock()
        self.model.apply_corrections("ws1", "out_ws", "dir", abs_corr=1.0, div_corr=1.0)

        mock_convert.assert_called_once_with(self.mock_ws, Target="dSpacing", StoreInADS=False)
        mock_clone.assert_called()
        mock_save.assert_called_once_with("out_ws", "dir", "AbsorptionCorrection", None, mock_calib.group)

    @patch(correction_model_path + ".TextureCorrectionModel._save_corrected_files")
    @patch(correction_model_path + ".ConvertUnits")
    @patch(correction_model_path + ".CloneWorkspace")
    @patch(correction_model_path + ".ADS")
    def test_apply_corrections_with_abs_and_div_workspaces(self, mock_ads, mock_clone, mock_convert, mock_save):
        # Arrange
        mock_calib = MagicMock()
        self.model.set_calibration(mock_calib)
        ws = MagicMock()
        abs_ws = MagicMock()
        div_ws = MagicMock()
        temp_ws = MagicMock()

        def retrieve_mock(name):
            return {"ws1": ws, "abs_ws": abs_ws, "div_ws": div_ws}[name]

        mock_ads.retrieve.side_effect = retrieve_mock
        mock_convert.side_effect = lambda x, Target, StoreInADS: temp_ws if x is ws else x

        self.model.apply_corrections("ws1", "out_ws", "dir", abs_corr="abs_ws", div_corr="div_ws")

        # Assert
        self.assertEqual(mock_convert.call_count, 2)
        mock_save.assert_called_once_with("out_ws", "dir", "AbsorptionCorrection", None, mock_calib.group)
        mock_ads.remove.assert_any_call("abs_ws")
        mock_ads.remove.assert_any_call("div_ws")

    @patch(correction_model_path + ".TextureCorrectionModel._save_corrected_files")
    @patch(correction_model_path + ".ConvertUnits")
    @patch(correction_model_path + ".CloneWorkspace")
    @patch(correction_model_path + ".ADS")
    def test_apply_corrections_with_remove_ws_flag(self, mock_ads, mock_clone, mock_convert, mock_save):
        mock_calib = MagicMock()
        self.model.set_calibration(mock_calib)
        ws = MagicMock()
        mock_ads.retrieve.return_value = ws
        mock_convert.return_value = ws
        self.model.set_remove_after_processing(True)

        self.model.apply_corrections("ws1", "out_ws", "dir", abs_corr=1.0, div_corr=1.0)

        mock_ads.remove.assert_any_call("out_ws")

    @patch(correction_model_path + ".TextureCorrectionModel._save_corrected_files")
    @patch(correction_model_path + ".ConvertUnits")
    @patch(correction_model_path + ".CloneWorkspace")
    @patch(correction_model_path + ".ADS")
    def test_apply_corrections_with_rb_number(self, mock_ads, mock_clone, mock_convert, mock_save):
        mock_calib = MagicMock()
        self.model.set_calibration(mock_calib)
        self.model.set_rb_num("rb123")
        ws = MagicMock()
        mock_ads.retrieve.return_value = ws
        mock_convert.return_value = ws
        mock_clone.return_value = None

        self.model.apply_corrections("ws1", "out_ws", "dir", abs_corr=1.0, div_corr=1.0)

        mock_save.assert_called_once_with("out_ws", "dir", "AbsorptionCorrection", "rb123", mock_calib.group)

    @patch(correction_model_path + ".SaveNexus")
    @patch(correction_model_path + ".CreateEmptyTableWorkspace")
    @patch(correction_model_path + ".ADS")
    def test_write_atten_val_table_creates_and_saves(self, mock_ads, mock_create, mock_save):
        mock_ads.retrieve.return_value.getNumberHistograms.return_value = 1
        mock_table = MagicMock()
        mock_create.return_value = mock_table
        with tempfile.TemporaryDirectory() as d:
            self.model.write_atten_val_table("ws1", [3.14], 2.0, "Wavelength", "rb123", mock.MagicMock(), d)
        mock_save.assert_called()

    @patch(correction_model_path + ".SaveNexus")
    @patch(correction_model_path + ".makedirs")
    @patch(correction_model_path + ".path.exists")
    def test_save_corrected_files_creates_expected_paths(self, mock_exists, mock_makedirs, mock_save):
        mock_exists.return_value = False
        calib = MagicMock()
        calib.group = "CUSTOM"
        with tempfile.TemporaryDirectory() as d:
            self.model._save_corrected_files("ws", d, "AbsorptionCorrection", "RB123", calib.group)
        mock_makedirs.assert_called()
        mock_save.assert_called()

    @patch(correction_model_path + ".LoadEmptyInstrument")
    def test_create_reference_ws_sets_correct_name_and_calls_loader(self, mock_load_instr):
        self.model.create_reference_ws("RB123")
        self.assertEqual(self.model.reference_ws, "RB123_reference_workspace")
        mock_load_instr.assert_called_once_with(InstrumentName="ENGINX", OutputWorkspace="RB123_reference_workspace")

    @patch(correction_model_path + ".TextureCorrectionModel.apply_corrections")
    @patch(correction_model_path + ".TextureCorrectionModel.calc_divergence")
    @patch(correction_model_path + ".TextureCorrectionModel.write_atten_val_table")
    @patch(correction_model_path + ".TextureCorrectionModel.read_attenuation_coefficient_at_value")
    @patch(correction_model_path + ".TextureCorrectionModel.calc_absorption")
    @patch(correction_model_path + ".TextureCorrectionModel.define_gauge_volume")
    def test_calc_all_corrections(
        self,
        mock_define_gauge_volume,
        mock_calc_absorption,
        mock_read_attenuation_coefficient_at_value,
        mock_write_atten_val_table,
        mock_calc_divergence,
        mock_apply_corrections,
    ):
        wss = [self.mock_ws]
        out_wss = ["Corrected_test1"]

        abs_args = {"gauge_vol_preset": "4mmCube", "gauge_vol_file": None, "mc_param_str": "SparseInstrument:True"}

        atten_args = {"atten_val": 2.0, "atten_units": "dSpacing"}

        div_args = {"hoz": 1.0, "vert": 1.0, "det_hoz": 1.0}

        rb = "rb123"
        root_dir = "dir"
        mock_calib = MagicMock()
        mock_atten_vals = MagicMock()
        mock_read_attenuation_coefficient_at_value.return_value = mock_atten_vals

        self.model.set_include_abs(True)
        self.model.set_include_atten(True)
        self.model.set_include_div(True)
        self.model.set_rb_num(rb)
        self.model.set_calibration(mock_calib)
        self.model.set_remove_after_processing(True)

        self.model.calc_all_corrections(wss, out_wss, root_dir, abs_args, atten_args, div_args)

        mock_define_gauge_volume.assert_called_once_with(self.mock_ws, abs_args["gauge_vol_preset"], abs_args["gauge_vol_file"])
        mock_calc_absorption.assert_called_once_with(self.mock_ws, abs_args["mc_param_str"])
        mock_read_attenuation_coefficient_at_value.assert_called_once_with("_abs_corr", atten_args["atten_val"], atten_args["atten_units"])
        mock_write_atten_val_table.assert_called_once_with(
            self.mock_ws,
            mock_atten_vals,
            atten_args["atten_val"],
            atten_args["atten_units"],
            rb,
            mock_calib,
            root_dir,
        )
        mock_calc_divergence.assert_called_once_with(self.mock_ws, div_args["hoz"], div_args["vert"], div_args["det_hoz"])
        mock_apply_corrections.assert_called_once_with(self.mock_ws, out_wss[0], root_dir, "_abs_corr", "_div_corr")


if __name__ == "__main__":
    unittest.main()
