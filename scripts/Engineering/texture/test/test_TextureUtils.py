import unittest
from unittest.mock import patch, MagicMock
from Engineering.texture.TextureUtils import (
    run_focus_script,
    run_abs_corr,
    validate_abs_corr_inputs,
    get_numerical_integ,
    fit_all_peaks,
    make_iterable,
    create_pf_loop,
)

texture_utils_path = "Engineering.texture.TextureUtils"


class TextureUtilsTest(unittest.TestCase):
    @patch(f"{texture_utils_path}.mk")
    @patch(f"{texture_utils_path}.TextureInstrument")
    def test_run_focus_script_instantiates_model_and_calls_main(self, mock_instr, mock_mk):
        mock_model = MagicMock()
        mock_instr.return_value = mock_model
        run_focus_script(wss=["1", "2"], focus_dir="focus", van_run="v", ceria_run="c", full_instr_calib="f", grouping="1")
        mock_instr.assert_called_once()
        mock_model.main.assert_called_once()
        mock_mk.assert_called_once_with("focus")

    @patch(f"{texture_utils_path}.TextureCorrectionModel")
    def test_run_abs_corr_executes_full_pipeline(self, mock_model_class):
        mock_model = MagicMock()
        mock_model_class.return_value = mock_model
        wss = ["ws1", "ws2"]
        run_abs_corr(
            wss=wss,
            ref_ws="ref",
            orientation_file="orient.txt",
            orient_file_is_euler=True,
            euler_scheme="xyz",
            euler_axes_sense="right",
            copy_ref=True,
            include_abs_corr=True,
            monte_carlo_args="Arg: Val",
            gauge_vol_preset="4mmCube",
            gauge_vol_shape_file="shape.xml",
            include_atten_table=True,
            eval_point="1.54",
            eval_units="Angstrom",
            exp_name="exp1",
            root_dir="/tmp",
            include_div_corr=True,
            div_hoz=1.0,
            div_vert=1.0,
            det_hoz=0.5,
            clear_ads_after=True,
        )

        mock_model.load_all_orientations.assert_called_once_with(wss, "orient.txt", True, "xyz", "right")
        mock_model.copy_sample_info.assert_called_once_with("ref", wss)
        self.assertEqual(mock_model.define_gauge_volume.call_count, 2)
        self.assertEqual(mock_model.calc_absorption.call_count, 2)
        self.assertEqual(mock_model.read_attenuation_coefficient_at_value.call_count, 2)
        self.assertEqual(mock_model.write_atten_val_table.call_count, 2)
        self.assertEqual(mock_model.calc_divergence.call_count, 2)
        self.assertEqual(mock_model.apply_corrections.call_count, 2)

    @patch(f"{texture_utils_path}.ADS")
    @patch(f"{texture_utils_path}.Fit")
    def test_get_numerical_integ_returns_expected_val_for_simple_peak(self, mock_fit, mock_ads):
        fit_ws = MagicMock()
        # data is three spectra: raw, bkg_fit, difference
        # this func will only be looking at diff (index : -1)
        fit_ws.extractX.return_value = [[0, 1, 2], [0, 1, 2], [0, 1, 2]]
        fit_ws.extractY.return_value = [[0, 3, 2], [0, 1, 2], [0, 2, 0]]
        mock_ads.retrieve.return_value = fit_ws

        result = get_numerical_integ("ws", 1)
        self.assertIsInstance(result, float)
        self.assertAlmostEqual(result, 2.0)  # data is a triangular peak A = 1/2(b*h) b= 2, h = 2

    def test_make_iterable_wraps_scalar(self):
        self.assertEqual(make_iterable("val"), ["val"])
        self.assertEqual(make_iterable(42), [42])
        self.assertEqual(make_iterable(["already", "list"]), ["already", "list"])
        self.assertEqual(make_iterable((1, 2)), (1, 2))

    @patch(f"{texture_utils_path}.ADS")
    @patch(f"{texture_utils_path}.get_spectrum_indices")
    @patch(f"{texture_utils_path}.CreateEmptyTableWorkspace")
    @patch(f"{texture_utils_path}.CropWorkspace")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}.SaveNexus")
    def test_fit_all_peaks_runs_fit_for_each_peak_and_spectrum(
        self, mock_save, mock_fit, mock_crop, mock_create_table, mock_spec_ind, mock_ads
    ):
        ws_mock = MagicMock()
        ws_mock.getNumberHistograms.return_value = 2
        run_prop = MagicMock()
        run_prop.value = "123456"
        ws_mock.getRun().getLogData.side_effect = lambda key: run_prop if key == "run_number" else MagicMock(value="GroupA")
        mock_ads.retrieve.side_effect = [ws_mock, MagicMock(), MagicMock()]
        mock_create_table.return_value = MagicMock()
        mock_spec_ind.return_value = [0]

        fit_all_peaks(["ws123456"], [1.54], 0.02, "/tmp/save", do_numeric_integ=False)
        self.assertTrue(mock_fit.called)
        self.assertTrue(mock_save.called)

    @patch(f"{texture_utils_path}.create_pf")
    def test_create_pf_loop_loads_and_calls_create_pf(self, mock_create):
        with patch(f"{texture_utils_path}.ADS.doesExist", return_value=False):
            create_pf_loop(
                wss=["ws1", "ws2"],
                param_wss=[
                    ["p1", "p2"],
                ],
                include_scatt_power=False,
                cif=None,
                lattice="2.8665  2.8665  2.8665",
                space_group="P1",
                basis="Fe 0 0 0 1.0 0.05",
                hkls=[[1, 1, 1], [1, 0, 0]],
                readout_columns="X0",
                dir1=[1, 0, 0],
                dir2=[0, 1, 0],
                dir3=[0, 0, 1],
                dir_names=["RD", "TD", "ND"],
                scatter=False,
                kernel=0.2,
                scat_vol_pos=[0, 0, 0],
                chi2_thresh=None,
                peak_thresh=None,
                save_root="/save",
                exp_name="exp",
                projection_method="Azimuthal",
            )
        mock_create.assert_called_once()

    def test_validate_abs_corr_inputs_when_orientation_file_given_requires_is_euler_flag(self):
        valid, error_msg = validate_abs_corr_inputs(orientation_file="some_file.txt")
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If orientation file is specified, must flag orient_file_is_euler.\n")

    def test_validate_abs_corr_inputs_when_is_euler_require_scheme_and_sense(self):
        valid, error_msg = validate_abs_corr_inputs(orientation_file="some_file.txt", orient_file_is_euler=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If orientation file is euler, must provide scheme and sense.\n")

    def test_validate_abs_corr_inputs_when_copy_ref_requires_ref_ws(self):
        valid, error_msg = validate_abs_corr_inputs(copy_ref=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If copy_ref is True, must provide ref_ws.\n")

    def test_validate_abs_corr_inputs_when_include_custom_gv_req_xml_file(self):
        valid, error_msg = validate_abs_corr_inputs(include_abs_corr=True, gauge_vol_preset="Custom")
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If custom gauge volume required, must provide shape xml as file.\n")

    def test_validate_abs_corr_inputs_when_attenuation_table_req_point_and_unit(self):
        valid, error_msg = validate_abs_corr_inputs(include_atten_table=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If attenuation table required, must provide valid point and units.\n")

    def test_validate_abs_corr_inputs_when_include_div_corr_require_divergence_vals(self):
        valid, error_msg = validate_abs_corr_inputs(include_div_corr=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If divergence correction required, must provide valid values.\n")


if __name__ == "__main__":
    unittest.main()
