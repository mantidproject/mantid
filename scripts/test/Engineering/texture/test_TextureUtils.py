# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call

import numpy as np
import tempfile
from Engineering.texture.TextureUtils import (
    find_all_files,
    mk,
    run_focus_script,
    run_abs_corr,
    validate_abs_corr_inputs,
    fit_all_peaks,
    make_iterable,
    create_pf_loop,
    _get_run_and_prefix_from_ws_log,
    _get_grouping_from_ws_log,
    TexturePeakFunctionGenerator,
)
from numpy import ones
import os

texture_utils_path = "Engineering.texture.TextureUtils"


class TextureUtilsTest(unittest.TestCase):
    def test_find_all_files_returns_only_files(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            file_path = os.path.join(tmpdir, "file1.txt")
            dir_path = os.path.join(tmpdir, "subdir")
            os.mkdir(dir_path)
            with open(file_path, "w") as f:
                f.write("test")
            result = find_all_files(tmpdir)
            self.assertIn(file_path, result)
            self.assertNotIn(dir_path, result)

    def test_mk_creates_missing_directory(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            test_path = os.path.join(tmpdir, "newdir")
            self.assertFalse(os.path.exists(test_path))
            mk(test_path)
            self.assertTrue(os.path.exists(test_path))

    @patch(f"{texture_utils_path}.mk")
    @patch(f"{texture_utils_path}.TextureInstrument")
    def test_run_focus_script_instantiates_model_and_calls_main(self, mock_instr, mock_mk):
        mock_model = MagicMock()
        mock_instr.return_value = mock_model
        run_focus_script(wss=["1", "2"], focus_dir="focus", van_run="v", ceria_run="c", full_instr_calib="f", grouping="1")
        mock_instr.assert_called_once()
        mock_model.main.assert_called_once()
        mock_mk.assert_called_once_with("focus")

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.validate_abs_corr_inputs")
    @patch(f"{texture_utils_path}.TextureCorrectionModel")
    def test_run_abs_corr_executes_full_pipeline(self, mock_model_class, mock_validate, mock_logger):
        # Arrange
        mock_model = MagicMock()
        mock_model_class.return_value = mock_model
        mock_validate.return_value = (True, "")

        wss = ["ws1", "ws2"]
        # Act
        with tempfile.TemporaryDirectory() as d:
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
                root_dir=d,
                include_div_corr=True,
                div_hoz=1.0,
                div_vert=1.0,
                det_hoz=0.5,
                clear_ads_after=True,
            )

        mock_validate.assert_called_once_with(
            "ref",
            "orient.txt",
            True,
            "xyz",
            "right",
            True,
            True,
            "4mmCube",
            "shape.xml",
            True,
            "1.54",
            "Angstrom",
            True,
            1.0,
            1.0,
            0.5,
        )
        mock_logger.error.assert_not_called()

        # Orientation file triggers a load
        mock_model.load_all_orientations.assert_called_once_with(wss, "orient.txt", True, "xyz", "right")

        # Copy ref when requested
        mock_model.copy_sample_info.assert_called_once_with("ref", wss)

        # Set processing flags / metadata
        mock_model.set_include_abs.assert_called_once_with(True)
        mock_model.set_include_atten.assert_called_once_with(True)
        mock_model.set_include_div.assert_called_once_with(True)
        mock_model.set_remove_after_processing.assert_called_once_with(True)

        # corrections call: check args and kwargs
        mock_model.calc_all_corrections.assert_called_once()
        args, kwargs = mock_model.calc_all_corrections.call_args

        # Positional args
        self.assertEqual(args[0], ["ws1", "ws2"])
        self.assertEqual(args[1], ["Corrected_ws1", "Corrected_ws2"])

        # Keyword args
        self.assertEqual(
            kwargs["abs_args"],
            {"gauge_vol_preset": "4mmCube", "gauge_vol_file": "shape.xml", "mc_param_str": "Arg: Val"},
        )
        self.assertEqual(kwargs["atten_args"], {"atten_val": "1.54", "atten_units": "Angstrom"})
        self.assertEqual(kwargs["div_args"], {"hoz": 1.0, "vert": 1.0, "det_hoz": 0.5})

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.TextureCorrectionModel")
    def test_run_abs_corr_logs_error_on_invalid_input(self, mock_model, mock_logger):
        run_abs_corr(
            wss=["ws1"],
            orientation_file="orient.txt",  # Missing orient_file_is_euler
        )
        mock_logger.error.assert_called_once()
        self.assertIn("must flag orient_file_is_euler", mock_logger.error.call_args[0][0])

    def test_get_run_and_prefix_with_valid_log(self):
        run_number = "123456"
        prefix = "TEST"
        ws_name = f"{prefix}{run_number}_ws"
        mock_ws = MagicMock()
        run = MagicMock()
        run.value = run_number
        mock_ws.getRun().getLogData.side_effect = lambda key: run if key == "run_number" else None  # no log data
        self.assertTrue(np.all((_get_run_and_prefix_from_ws_log(mock_ws, ws_name), (run_number, prefix))))

    def test_get_run_and_prefix_with_no_log_gives_fallback(self):
        ws_name = "TEST123456_ws"
        mock_ws = MagicMock()
        mock_ws.getRun().getLogData.side_effect = RuntimeError()  # no log data
        out_run, out_prefix = _get_run_and_prefix_from_ws_log(mock_ws, ws_name)
        self.assertEqual(out_run, "unknown")
        self.assertEqual(out_prefix, "")

    def test_get_grouping_with_valid_log(self):
        grouping = "Test"
        mock_ws = MagicMock()
        group = MagicMock()
        group.value = grouping
        mock_ws.getRun().getLogData.side_effect = lambda key: group if key == "Grouping" else None  # no log data
        self.assertEqual(_get_grouping_from_ws_log(mock_ws), grouping)

    def test_get_grouping_with_no_log_gives_fallback(self):
        mock_ws = MagicMock()
        mock_ws.getRun().getLogData.side_effect = RuntimeError()
        out_group = _get_grouping_from_ws_log(mock_ws)
        self.assertEqual(out_group, "GROUP")

    def setup_expected_fit_results(self, wsname, prefix, run_number, group, peak, num_spec, save_dir):
        expected_func = "some_fit_func"
        expected_first_fit_kwargs = {
            "Function": expected_func,
            "CostFunction": "Least squares",
            "Output": "first_fit",
            "MaxIterations": 50,
            "InputWorkspace": wsname,
            "Minimizer": "Levenberg-Marquardt",
            "StepSizeMethod": "Sqrt epsilon",
            "IgnoreInvalidData": True,
            "CreateOutput": True,
            "OutputCompositeMembers": True,
        }
        expected_fit_kwargs = {
            "Function": expected_func,
            "CostFunction": "Least squares",
            "Output": "fit",
            "MaxIterations": 50,
            "InputWorkspace": wsname,
            "Minimizer": "Levenberg-Marquardt",
            "StepSizeMethod": "Sqrt epsilon",
            "IgnoreInvalidData": True,
            "CreateOutput": True,
            "OutputCompositeMembers": True,
        }
        expected_tab_name = f"{prefix}{run_number}_{peak}_{group}_Fit_Parameters"
        expected_func_kwargs = {"InputWorkspace": wsname}
        expected_create_tab_kwargs = {"OutputWorkspace": expected_tab_name}
        expected_save_kwargs = {"InputWorkspace": expected_tab_name, "Filename": os.path.join(save_dir, f"{expected_tab_name}.nxs")}
        expected_intensity_sub_bkg = ones(num_spec)
        # expect a row for each parameter of each function for each spectra - here we just have the one func f0 with param P
        # also expect a final cost function
        expected_name_col = [f"f{i}.f0.P" for i in range(num_spec)] + ["Cost Function"]
        expected_val_col = np.ones(len(expected_name_col))
        expected_err_col = np.zeros_like(expected_val_col)

        return {
            "wsname": wsname,
            "prefix": prefix,
            "run_number": run_number,
            "group": group,
            "peak": peak,
            "num_spec": peak,
            "save_dir": save_dir,
            "expected_func": expected_func,
            "expected_first_fit_kwargs": expected_first_fit_kwargs,
            "expected_fit_kwargs": expected_fit_kwargs,
            "expected_func_kwargs": expected_func_kwargs,
            "expected_create_tab_kwargs": expected_create_tab_kwargs,
            "expected_save_kwargs": expected_save_kwargs,
            "expected_name_col": expected_name_col,
            "expected_val_col": expected_val_col,
            "expected_err_col": expected_err_col,
            "expected_intensity_sub_bkg": expected_intensity_sub_bkg,
        }

    def setup_fit_mocks_from_expected_results(self, repeated_expected_results, mock_peak_func_gen_cls, mock_ads, mock_intens_sigma):
        # mock ws information retrieval
        mock_ws = MagicMock()
        mock_ws.getNumberHistograms.side_effect = [er["num_spec"] for er in repeated_expected_results]

        # mock Peak Function Generator
        mock_gen = MagicMock()
        mock_gen.get_initial_fit_function_and_kwargs_from_specs.side_effect = [
            (er["expected_func"], er["expected_func_kwargs"], er["expected_intensity_sub_bkg"]) for er in repeated_expected_results
        ]
        mock_gen.get_final_fit_function.side_effect = [er["expected_func"] for er in repeated_expected_results]
        mock_peak_func_gen_cls.return_value = mock_gen

        # mock Output Parameter Table
        mock_param_ws = MagicMock()
        mock_get_name = MagicMock()
        mock_get_val = MagicMock()
        mock_get_err = MagicMock()
        mock_get_name.side_effect = [er["expected_name_col"] for er in repeated_expected_results]
        mock_get_val.side_effect = [er["expected_val_col"] for er in repeated_expected_results]
        mock_get_err.side_effect = [er["expected_err_col"] for er in repeated_expected_results]
        mock_param_ws.column.side_effect = lambda name: (
            mock_get_name() if name == "Name" else mock_get_val() if name == "Value" else mock_get_err()
        )

        # mock ADS calls (first to get ws, then to get parameter table)
        mock_ads.retrieve.side_effect = [mock_ws, mock_param_ws] * len(repeated_expected_results)
        mock_intens_sigma.return_value = (None, np.ones(30), None)
        return mock_ws, mock_param_ws, mock_peak_func_gen_cls, mock_ads

    @patch(f"{texture_utils_path}.SaveNexus")
    @patch(f"{texture_utils_path}.CreateEmptyTableWorkspace")
    @patch(f"{texture_utils_path}.calc_intens_and_sigma_arrays")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}._get_grouping_from_ws_log")
    @patch(f"{texture_utils_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{texture_utils_path}.ADS")
    @patch(f"{texture_utils_path}.TexturePeakFunctionGenerator")
    def test_fit_all_peaks_basic_fit(
        self,
        mock_peak_func_gen_cls,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_fit,
        mock_intens_sigma,
        mock_create_tab_ws,
        mock_save_nexus,
    ):
        # GIVEN
        wsname = "TEST123456_ws"
        prefix = "TEST"
        run_number = "123456"
        group = "TestGroup"
        peak = 1.0
        save_dir = "save"
        num_spec = 2

        # EXPECT
        expected_results = self.setup_expected_fit_results(wsname, prefix, run_number, group, peak, num_spec, save_dir)

        # SETUP TEST
        mocks = self.setup_fit_mocks_from_expected_results((expected_results,), mock_peak_func_gen_cls, mock_ads, mock_intens_sigma)
        mock_ws, mock_param_ws, mock_peak_func_gen_cls, mock_ads = mocks
        mock_get_run_prefix.return_value = (run_number, prefix)
        mock_get_group.return_value = group

        # TEST
        fit_all_peaks(
            wss=[
                wsname,
            ],
            peaks=[peak],
            peak_window=0.1,
            save_dir=save_dir,
            override_dir=True,
        )

        # CHECK
        mock_create_tab_ws.assert_called_once_with(**expected_results["expected_create_tab_kwargs"])
        mock_fit.assert_has_calls(
            [call(**expected_results["expected_first_fit_kwargs"]), call(**expected_results["expected_fit_kwargs"])], any_order=True
        )
        mock_save_nexus.assert_called_once_with(**expected_results["expected_save_kwargs"])

    @patch(f"{texture_utils_path}.SaveNexus")
    @patch(f"{texture_utils_path}.CreateEmptyTableWorkspace")
    @patch(f"{texture_utils_path}.calc_intens_and_sigma_arrays")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}._get_grouping_from_ws_log")
    @patch(f"{texture_utils_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{texture_utils_path}.ADS")
    @patch(f"{texture_utils_path}.TexturePeakFunctionGenerator")
    def test_fit_all_peaks_multiple_wss_and_peaks(
        self,
        mock_peak_func_gen_cls,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_fit,
        mock_intens_sigma,
        mock_create_tab_ws,
        mock_save_nexus,
    ):
        # GIVEN
        wss = ["TEST000101_ws", "TEST000102_ws"]
        runs = ["000101", "000102"]
        prefix = "TEST"
        group = "TestGroup"
        peaks = [1.0, 2.0]
        save_dir = "save"
        num_spec = 2

        all_expected = []
        for wsname, run_number in zip(wss, runs):
            for peak in peaks:
                expected = self.setup_expected_fit_results(wsname, prefix, run_number, group, peak, num_spec, save_dir)
                all_expected.append(expected)

        mocks = self.setup_fit_mocks_from_expected_results(all_expected, mock_peak_func_gen_cls, mock_ads, mock_intens_sigma)
        mock_ws, mock_param_ws, mock_peak_func_gen_cls, mock_ads = mocks
        mock_get_run_prefix.side_effect = [(run_number, prefix) for run_number in runs]
        mock_get_group.side_effect = [group for _ in runs]

        # TEST
        fit_all_peaks(wss=wss, peaks=peaks, peak_window=0.1, save_dir=save_dir, override_dir=True)

        # CHECK
        self.assertEqual(mock_fit.call_count, len(all_expected) * 2)  # first fit and second fit
        self.assertEqual(mock_create_tab_ws.call_count, len(all_expected))
        self.assertEqual(mock_save_nexus.call_count, len(all_expected))

        expected_create_calls = [call(**exp["expected_create_tab_kwargs"]) for exp in all_expected]
        expected_fit_calls = [call(**exp["expected_first_fit_kwargs"]) for exp in all_expected] + [
            call(**exp["expected_fit_kwargs"]) for exp in all_expected
        ]
        expected_save_calls = [call(**exp["expected_save_kwargs"]) for exp in all_expected]

        mock_create_tab_ws.assert_has_calls(expected_create_calls, any_order=True)
        mock_fit.assert_has_calls(expected_fit_calls, any_order=True)
        mock_save_nexus.assert_has_calls(expected_save_calls, any_order=False)

    def test_make_iterable_wraps_scalar(self):
        self.assertEqual(make_iterable("val"), ["val"])
        self.assertEqual(make_iterable(42), [42])
        self.assertEqual(make_iterable(["already", "list"]), ["already", "list"])
        self.assertEqual(make_iterable((1, 2)), (1, 2))

    @patch(f"{texture_utils_path}.TexturePeakFunctionGenerator._estimate_intensity_and_background")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.ADS")
    def test_get_initial_fit_function_and_kwargs_from_specs_raises_on_invalid_tie_param(self, mock_ads, mock_factory, mock_estimate):
        mock_ws = MagicMock()
        si = MagicMock()
        si.size.return_value = 1
        mock_ws.spectrumInfo.return_value = si
        mock_ws.name.return_value = "test_ws"
        mock_ws.yIndexOfX.side_effect = lambda x: 0
        mock_ads.retrieve.return_value = mock_ws

        mock_estimate.return_value = ((1, 2), 0, 0)

        mock_peak_func = MagicMock()
        mock_peak_func.getCentreParameterName.return_value = "X0"
        mock_peak_func.getWidthParameterName.return_value = "Sig"
        mock_peak_func.getParamName.side_effect = lambda i: ["A", "B", "X0", "Sig"][i]
        mock_peak_func.nParams.return_value = 4
        mock_peak_func.isExplicitlySet.side_effect = lambda i: i == 0

        mock_factory.Instance().createPeakFunction.return_value = mock_peak_func
        mock_factory.createFunction.return_value = MagicMock()

        gen = TexturePeakFunctionGenerator([])
        with self.assertRaises(ValueError) as ctx:
            gen.get_initial_fit_function_and_kwargs_from_specs(
                mock_ws,
                peak=1.0,
                x_window=(0.9, 1.1),
                parameters_to_tie=["NotAParam"],
                peak_func_name="BackToBackExponential",
                bg_func_name="LinearBackground",
            )
            self.assertIn("Invalid parameter(s) to tie", str(ctx.exception))

    def get_default_pf_kwargs(self):
        return {
            "wss": ["ws1"],
            "param_wss": [["p1"]],
            "include_scatt_power": False,
            "xtal_input": None,
            "hkls": [[1, 1, 1]],
            "readout_columns": ["X0", "I"],
            "dir1": [1, 0, 0],
            "dir2": [0, 1, 0],
            "dir3": [0, 0, 1],
            "dir_names": ["RD", "TD", "ND"],
            "scatter": "both",
            "kernel": 0.2,
            "scat_vol_pos": [0, 0, 0],
            "chi2_thresh": None,
            "peak_thresh": None,
            "save_root": "/save",
            "exp_name": "exp",
            "projection_method": "Azimuthal",
        }

    @patch(f"{texture_utils_path}.create_pf")
    def test_create_pf_loop_scatter_both_calls_create_twice_per_column(self, mock_create):
        create_pf_loop(**self.get_default_pf_kwargs())
        # 2 columns * 2 scatter variants = 4 calls
        self.assertEqual(mock_create.call_count, 4)

    @patch(f"{texture_utils_path}.create_pf")
    def test_create_pf_loop_loads_and_calls_create_pf(self, mock_create):
        with patch(f"{texture_utils_path}.ADS.doesExist", return_value=False):
            args = self.get_default_pf_kwargs()
            args["scatter"] = True
            args["wss"] = ["ws1", "ws2"]
            args["param_wss"] = (["p1", "p2"],)
            args["hkls"] = [[1, 1, 1], [1, 0, 0]]
            args["readout_columns"] = ["I"]
            create_pf_loop(**args)
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
