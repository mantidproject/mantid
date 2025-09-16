import unittest
from unittest.mock import patch, MagicMock
from os import path
import numpy as np
import tempfile
from Engineering.texture.TextureUtils import (
    find_all_files,
    mk,
    run_focus_script,
    run_abs_corr,
    validate_abs_corr_inputs,
    fit_all_peaks,
    get_initial_fit_function_and_kwargs_from_specs,
    create_pf_loop,
    get_xtal_structure,
    _get_run_and_prefix_from_ws_log,
    _get_grouping_from_ws_log,
)
from numpy import ones
import os

texture_utils_path = "Engineering.texture.TextureUtils"


class CrystalPhaseHelperMixin:
    def get_valid_xtal_string_kwargs(self):
        return {"lattice": "2.8665  2.8665  2.8665", "space_group": "I m -3 m", "basis": "Fe 0 0 0 1.0 0.05; Fe 0.5 0.5 0.5 1.0 0.05"}

    def get_valid_xtal_array_kwargs(self):
        return {"alatt": ones(3), "space_group": "I m -3 m", "basis": "Fe 0 0 0 1.0 0.05; Fe 0.5 0.5 0.5 1.0 0.05"}

    def get_valid_xtal_cif_kwargs(self):
        return {"cif_file": "example.cif"}

    def get_invalid_kwargs_dict(self, valid_kwarg_dict, key):
        valid_kwarg_dict["bad_val"] = valid_kwarg_dict.pop(key)
        return valid_kwarg_dict

    def get_invalid_kwargs_val_dict(self, valid_kwarg_dict, key, bad_val):
        valid_kwarg_dict[key] = bad_val
        return valid_kwarg_dict

    def exec_kwarg_checker(self, valid_kwargs_dict, input_type):
        valid_kwargs = list(valid_kwargs_dict.keys())
        for key in valid_kwargs:
            bad_kwargs_dict = self.get_invalid_kwargs_dict(valid_kwargs_dict, key)
            self.assertFalse(self.try_make_xtal_from_args(input_type, bad_kwargs_dict))

    def try_make_xtal_from_args(self, input_type, kwargs_dict):
        try:
            get_xtal_structure(input_type, **kwargs_dict)
            return True
        except Exception:
            return False

    def exec_bad_val_checker(self, valid_kwargs_dict, input_type, bad_val):
        valid_kwargs = valid_kwargs_dict.keys()
        for key in valid_kwargs:
            bad_kwargs_dict = self.get_invalid_kwargs_val_dict(valid_kwargs_dict, key, bad_val)
            self.assertFalse(self.try_make_xtal_from_args(input_type, bad_kwargs_dict))


class TextureUtilsTest(CrystalPhaseHelperMixin, unittest.TestCase):
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

    # --- helpers for current fitting flow ---

    def _make_param_table_mock(self, num_spec: int, params=("A", "B", "S", "X0")):
        names = [f"f{ispec}.f0.{p}" for ispec in range(num_spec) for p in params] + ["Cost Function"]
        vals = np.arange(1, len(names) + 1, dtype=float)  # just distinct values
        errs = np.full_like(vals, 0.1)
        param_ws = MagicMock()

        def _col(which):
            if which == "Name":
                return names
            if which == "Value":
                return vals
            return errs

        param_ws.column.side_effect = _col
        return param_ws, names, vals, errs

    @patch(f"{texture_utils_path}.SaveNexus")
    @patch(f"{texture_utils_path}.CreateEmptyTableWorkspace")
    @patch(
        f"{texture_utils_path}.calc_intens_and_sigma_arrays",
        return_value=(None, None, np.array([3.5, 5.0]), None),
    )
    @patch(f"{texture_utils_path}._convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.rerun_fit_with_new_ws")
    @patch(f"{texture_utils_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}.Rebunch")
    @patch(f"{texture_utils_path}.ConvertUnits")
    @patch(f"{texture_utils_path}._get_grouping_from_ws_log")
    @patch(f"{texture_utils_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{texture_utils_path}.ADS")
    def test_fit_all_peaks_basic_fit(
        self,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_convert_units,
        mock_rebunch,
        mock_fit,
        mock_get_initial,
        mock_rerun_with_new,
        mock_unitconv,
        mock_convert_toferr_to_derr,
        _mock_calc_i_over_sig,
        mock_create_tab_ws,
        mock_save_nexus,
    ):
        mock_unitconv.run.return_value = 1.2345
        mock_convert_toferr_to_derr.return_value = 0.01

        wsname = "TEST123456_ws"
        prefix, run_number, group = "TEST", "123456", "TestGroup"
        peak, save_dir, num_spec = 1.0, "save", 2

        ws = MagicMock()
        si = MagicMock()
        si.size.return_value = num_spec
        ws.spectrumInfo.return_value = si

        mock_convert_units.return_value = "ws_tof"
        mock_rebunch.side_effect = ["smooth_ws_3", "smooth_ws_2"]

        md_fit_kwargs = {"InputWorkspace": "ws_tof", "StartX": 0.9, "EndX": 1.1, "WorkspaceIndex": 0}
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0], 0.1)

        fit_obj = MagicMock()
        fit_obj.Function.function = "md_function"
        fit_obj.OutputWorkspace.name.return_value = "fit_out_ws"
        mock_fit.return_value = fit_obj

        fit_obj2 = MagicMock()
        fit_obj2.Function.function = "md_function_final"
        fit_obj2.OutputWorkspace.name.return_value = "fit_out_ws_final"
        mock_rerun_with_new.return_value = (fit_obj2, md_fit_kwargs)

        param_ws, *_ = self._make_param_table_mock(num_spec, params=())
        mock_ads.retrieve.side_effect = [ws, param_ws]

        mock_get_run_prefix.return_value = (run_number, prefix)
        mock_get_group.return_value = group

        tab_ws = MagicMock()
        mock_create_tab_ws.return_value = tab_ws

        fit_all_peaks(
            wss=[wsname],
            peaks=[peak],
            peak_window=0.1,
            save_dir=save_dir,
            override_dir=True,
        )

        expected_tab_name = f"{prefix}{run_number}_{peak}_{group}_Fit_Parameters"
        expected_out_path = path.join(save_dir, f"{expected_tab_name}.nxs")
        mock_create_tab_ws.assert_called_once_with(OutputWorkspace=expected_tab_name)
        mock_save_nexus.assert_called_once_with(InputWorkspace=expected_tab_name, Filename=expected_out_path)

        mock_get_initial.assert_called_once_with(
            ws,
            "smooth_ws_3",
            peak,
            (peak - 0.1, peak + 0.1),
            ("A", "B"),
            "BackToBackExponential",
            "LinearBackground",
            False,
        )

        _, fit_call_kwargs = mock_fit.call_args
        self.assertEqual(fit_call_kwargs["Function"], "FUNC")
        self.assertEqual(fit_call_kwargs["Output"], "fit_smooth_ws_3")
        self.assertEqual(fit_call_kwargs["MaxIterations"], 50)
        self.assertEqual(fit_call_kwargs["CostFunction"], "Unweighted least squares")

        mock_rerun_with_new.assert_called_once()
        rerun_args, _ = mock_rerun_with_new.call_args
        self.assertEqual(rerun_args[3], "smooth_ws_2")
        self.assertEqual(tab_ws.addRow.call_count, num_spec)

    # -------- MULTIPLE wss & peaks --------

    @patch(f"{texture_utils_path}.SaveNexus")
    @patch(f"{texture_utils_path}.CreateEmptyTableWorkspace")
    @patch(
        f"{texture_utils_path}.calc_intens_and_sigma_arrays",
        return_value=(None, None, np.array([3.0, 3.0]), None),
    )
    @patch(f"{texture_utils_path}._convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.rerun_fit_with_new_ws")
    @patch(f"{texture_utils_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}.Rebunch")
    @patch(f"{texture_utils_path}.ConvertUnits")
    @patch(f"{texture_utils_path}._get_grouping_from_ws_log")
    @patch(f"{texture_utils_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{texture_utils_path}.ADS")
    def test_fit_all_peaks_multiple_wss_and_peaks(
        self,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_convert_units,
        mock_rebunch,
        mock_fit,
        mock_get_initial,
        mock_rerun_with_new,
        mock_unitconv,
        mock_convert_toferr_to_derr,
        _mock_calc_i_over_sig,
        mock_create_tab_ws,
        mock_save_nexus,
    ):
        mock_unitconv.run.return_value = 1.2345
        mock_convert_toferr_to_derr.return_value = 0.01

        wss = ["TEST000101_ws", "TEST000102_ws"]
        runs = ["000101", "000102"]
        prefix, group, peaks, num_spec = "TEST", "TestGroup", [1.0, 2.0], 2

        ws1, ws2 = MagicMock(), MagicMock()
        si1, si2 = MagicMock(), MagicMock()
        si1.size.return_value = num_spec
        si2.size.return_value = num_spec
        ws1.spectrumInfo.return_value = si1
        ws2.spectrumInfo.return_value = si2

        mock_convert_units.side_effect = ["ws_tof_1", "ws_tof_2"]
        mock_rebunch.side_effect = ["smooth_ws_3_1", "smooth_ws_2_1", "smooth_ws_3_2", "smooth_ws_2_2"]

        md_fit_kwargs = {"InputWorkspace": "ws_tof", "StartX": 0.9, "EndX": 1.1, "WorkspaceIndex": 0}
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0], 0.1)

        fit_obj = MagicMock()
        fit_obj.Function.function = "md_function"
        fit_obj.OutputWorkspace.name.return_value = "fit_out_ws"
        mock_fit.return_value = fit_obj
        mock_rerun_with_new.return_value = (fit_obj, md_fit_kwargs)

        # param tables with only cost row
        param_ws, *_ = self._make_param_table_mock(num_spec, params=())
        mock_ads.retrieve.side_effect = [ws1, param_ws, param_ws, ws2, param_ws, param_ws]

        mock_get_run_prefix.side_effect = [(runs[0], prefix), (runs[1], prefix)]
        mock_get_group.side_effect = [group, group]

        fit_all_peaks(
            wss=wss,
            peaks=peaks,
            peak_window=0.1,
            save_dir="save",
            override_dir=True,
        )

        expected_calls = len(wss) * len(peaks)
        self.assertEqual(mock_fit.call_count, expected_calls)
        self.assertEqual(mock_rerun_with_new.call_count, expected_calls)
        self.assertEqual(mock_create_tab_ws.call_count, expected_calls)
        self.assertEqual(mock_save_nexus.call_count, expected_calls)

    # -------- invalid tie parameter --------

    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.MultiDomainFunction")
    def test_get_initial_fit_function_and_kwargs_from_specs_raises_on_invalid_tie_param(self, mock_mdf, mock_factory):
        ws = MagicMock()
        si = MagicMock()
        si.size.return_value = 0  # skip per-spectrum loop
        ws.spectrumInfo.return_value = si
        ws_tof = MagicMock()

        # peak function with known available params
        peak_func = MagicMock()
        peak_func.nParams.return_value = 3
        peak_func.getParamName.side_effect = lambda i: ["I", "X0", "S"][i]
        peak_func.isExplicitlySet.side_effect = lambda i: i == 0  # "I" is intensity parameter
        peak_func.getCentreParameterName.return_value = "X0"
        peak_func.getWidthParameterName.return_value = "S"

        # factory wiring
        mock_factory.Instance().createPeakFunction.return_value = peak_func
        mock_factory.createFunction.return_value = MagicMock()

        # MDF minimal behavior
        mdf = MagicMock()
        mdf.nDomains.return_value = 0
        mock_mdf.return_value = mdf

        # invalid parameter name
        with self.assertRaises(ValueError) as ctx:
            get_initial_fit_function_and_kwargs_from_specs(
                ws=ws,
                ws_tof=ws_tof,
                peak=1.0,
                x_window=(0.9, 1.1),
                parameters_to_tie=["NotAParam"],
                peak_func_name="BackToBackExponential",
                bg_func_name="LinearBackground",
                tie_bkg=False,
            )
        self.assertIn("Invalid parameter(s) to tie", str(ctx.exception))

    def test_make_xtal_from_string(self):
        string_kwargs = self.get_valid_xtal_string_kwargs()
        self.assertTrue(self.try_make_xtal_from_args("string", string_kwargs))

    def test_make_phase_from_array(self):
        array_kwargs = self.get_valid_xtal_array_kwargs()
        self.assertTrue(self.try_make_xtal_from_args("array", array_kwargs))

    @patch(f"{texture_utils_path}.LoadCIF")
    @patch(f"{texture_utils_path}.CreateSingleValuedWorkspace")
    def test_make_xtal_from_cif(self, mock_create, mock_load):
        cif_kwargs = self.get_valid_xtal_cif_kwargs()

        # setup mocks for the cif loading
        mock_ws = MagicMock()
        mock_sample = MagicMock()
        mock_ws.sample().return_value = mock_sample
        mock_create.return_value = mock_ws
        mock_load.return_value = MagicMock()

        # check
        self.assertTrue(self.try_make_xtal_from_args("cif", cif_kwargs))
        mock_create.assert_called_once()
        mock_load.assert_called_once()

    def test_make_xtal_from_string_gives_error_invalid_kwarg(self):
        self.exec_kwarg_checker(self.get_valid_xtal_string_kwargs(), "string")

    def test_make_xtal_from_array_gives_error_invalid_kwarg(self):
        self.exec_kwarg_checker(self.get_valid_xtal_array_kwargs(), "array")

    def test_make_xtal_from_cif_gives_error_invalid_kwarg(self):
        self.exec_kwarg_checker(self.get_valid_xtal_cif_kwargs(), "cif")

    def test_make_xtal_from_string_gives_error_bad_vals(self):
        self.exec_bad_val_checker(self.get_valid_xtal_string_kwargs(), "string", "test")

    def test_make_xtal_from_array_gives_error_bad_vals(self):
        self.exec_bad_val_checker(self.get_valid_xtal_string_kwargs(), "array", "test")

    def test_get_xtal_structure_invalid_input_method_raises(self):
        with self.assertRaises(ValueError):
            get_xtal_structure("invalid_method")

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
