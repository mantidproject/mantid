# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call
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
    _get_run_and_prefix_from_ws_log,
    _get_grouping_from_ws_log,
    _get_instrument_from_ws_list,
    fit_initial_summed_spectra,
    rerun_fit_with_new_ws,
    _tie_bkg,
    crop_and_rebin,
    crop_wss_and_combine,
    _make_composite,
    _get_default_param_ties,
    calc_intens_and_sigma_arrays,
)
import os

texture_utils_path = "Engineering.texture.TextureUtils"


class TextureUtilsFileHelperTests(unittest.TestCase):
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


class TestGetInstrumentFromWsList(unittest.TestCase):
    @patch(f"{texture_utils_path}.ADS")
    def test_single_ws_in_ads_returns_instrument_name(self, mock_ads):
        mock_ws = MagicMock()
        mock_ws.getInstrument().getName.return_value = "ENGINX"
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.return_value = mock_ws

        result = _get_instrument_from_ws_list(["ws1"])

        mock_ads.doesExist.assert_called_once_with("ws1")
        mock_ads.retrieve.assert_called_once_with("ws1")
        self.assertEqual(result, "ENGINX")

    @patch(f"{texture_utils_path}.ADS")
    def test_multiple_ws_same_instrument_in_ads_returns_instrument_name(self, mock_ads):
        mock_ws = MagicMock()
        mock_ws.getInstrument().getName.return_value = "ENGINX"
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.return_value = mock_ws

        result = _get_instrument_from_ws_list(["ws1", "ws2", "ws3"])

        self.assertEqual(result, "ENGINX")
        self.assertEqual(mock_ads.retrieve.call_count, 3)

    @patch(f"{texture_utils_path}.Load")
    @patch(f"{texture_utils_path}.ADS")
    def test_ws_not_in_ads_loads_from_file(self, mock_ads, mock_load):
        mock_ws = MagicMock()
        mock_ws.getInstrument().getName.return_value = "IMAT"
        mock_ads.doesExist.return_value = False
        mock_load.return_value = mock_ws

        result = _get_instrument_from_ws_list(["path/to/file.nxs"])

        mock_load.assert_called_once_with(Filename="path/to/file.nxs")
        self.assertEqual(result, "IMAT")

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.Load")
    @patch(f"{texture_utils_path}.ADS")
    def test_ws_not_loadable_logs_error_and_returns_none(self, mock_ads, mock_load, mock_logger):
        mock_ads.doesExist.return_value = False
        mock_load.side_effect = RuntimeError("Cannot load file")

        result = _get_instrument_from_ws_list(["nonexistent.nxs"])

        mock_logger.error.assert_called_once()
        self.assertIn("nonexistent.nxs", mock_logger.error.call_args[0][0])
        self.assertIsNone(result)

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.ADS")
    def test_multiple_instruments_logs_error_and_returns_none(self, mock_ads, mock_logger):
        mock_ws1 = MagicMock()
        mock_ws1.getInstrument().getName.return_value = "ENGINX"
        mock_ws2 = MagicMock()
        mock_ws2.getInstrument().getName.return_value = "IMAT"
        mock_ads.doesExist.return_value = True
        mock_ads.retrieve.side_effect = [mock_ws1, mock_ws2]

        result = _get_instrument_from_ws_list(["ws1", "ws2"])

        mock_logger.error.assert_called_once()
        self.assertIn("multiple different instruments", mock_logger.error.call_args[0][0])
        self.assertIsNone(result)

    @patch(f"{texture_utils_path}.logger")
    def test_empty_list_logs_error_and_returns_none(self, mock_logger):
        result = _get_instrument_from_ws_list([])

        mock_logger.error.assert_called_once()
        self.assertIsNone(result)

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.Load")
    @patch(f"{texture_utils_path}.ADS")
    def test_load_failure_stops_processing_remaining_ws(self, mock_ads, mock_load, mock_logger):
        # If the second ws fails to load, processing stops early and returns None.
        mock_ws1 = MagicMock()
        mock_ws1.getInstrument().getName.return_value = "ENGINX"
        mock_ads.doesExist.side_effect = [True, False]
        mock_ads.retrieve.return_value = mock_ws1
        mock_load.side_effect = RuntimeError("Cannot load")

        result = _get_instrument_from_ws_list(["ws1", "bad_file.nxs"])

        mock_logger.error.assert_called_once()
        self.assertIn("bad_file.nxs", mock_logger.error.call_args[0][0])
        self.assertIsNone(result)


class TextureUtilsFocusTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.mk")
    @patch(f"{texture_utils_path}.IMAT")
    @patch(f"{texture_utils_path}.EnginX")
    @patch(f"{texture_utils_path}._get_instrument_from_ws_list", return_value="ENGINX")
    def test_run_focus_script_instantiates_ENGINX_model_and_calls_main(self, mock_get_instr, mock_enginx, mock_imat, mock_mk):
        run_focus_script(wss=["1", "2"], focus_dir="focus", van_run="v", ceria_run="c", full_instr_calib="f", grouping="1")
        mock_enginx.return_value.main.assert_called_once()
        mock_imat.return_value.main.assert_not_called()
        mock_mk.assert_called_once_with("focus")

    @patch(f"{texture_utils_path}.mk")
    @patch(f"{texture_utils_path}.IMAT")
    @patch(f"{texture_utils_path}.EnginX")
    @patch(f"{texture_utils_path}._get_instrument_from_ws_list", return_value="IMAT")
    def test_run_focus_script_instantiates_IMAT_model_and_calls_main(self, mock_get_instr, mock_enginx, mock_imat, mock_mk):
        run_focus_script(wss=["1", "2"], focus_dir="focus", van_run="v", ceria_run="c", full_instr_calib="f", grouping="1")
        mock_enginx.return_value.main.assert_not_called()
        mock_imat.return_value.main.assert_called_once()
        mock_mk.assert_called_once_with("focus")


class TextureUtilsAbsCorrTests(unittest.TestCase):
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


class TextureUtilsFittingUtilsTests(unittest.TestCase):
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

    def test_tie_bkg(self):
        # inputs
        function = MagicMock()
        approx_bkgs = [1.0, 3.0]
        ties = []

        bg1 = MagicMock()
        bg1.nParams.return_value = 2
        bg1.getParamName.side_effect = ("A0", "A1")

        bg0 = {}

        comp0, comp1 = MagicMock(), MagicMock()
        comp0.__getitem__.return_value = bg0
        comp1.__getitem__.return_value = bg1

        function.__getitem__.side_effect = lambda i: [comp0, comp1][i]
        function.nDomains.return_value = 2

        # exec
        out_func, out_ties = _tie_bkg(function, approx_bkgs, ties)

        self.assertIs(out_func, function)
        self.assertIs(out_ties, ties)
        self.assertEqual(out_ties, ["f1.f1.A0=f0.f1.A0", "f1.f1.A1=f0.f1.A1"])
        self.assertEqual(bg0["A0"], 2.0)

    @patch(f"{texture_utils_path}.Rebin")
    @patch(f"{texture_utils_path}.CropWorkspace")
    def test_crop_and_rebin(self, mock_crop, mock_rebin):
        # inputs
        ws = "ws1"
        out = "out_ws"
        lower = 1
        upper = 2
        rebin_params = (1, 0.1, 2)

        crop_and_rebin(ws, out, lower, upper, rebin_params)

        mock_crop.assert_called_once_with(ws, lower, upper, OutputWorkspace="__tmp_peak_window")
        mock_rebin.assert_called_once_with("__tmp_peak_window", rebin_params, OutputWorkspace=out)

    @patch(f"{texture_utils_path}.SumSpectra")
    @patch(f"{texture_utils_path}.AppendSpectra")
    @patch(f"{texture_utils_path}.CloneWorkspace")
    @patch(f"{texture_utils_path}.Rebin")
    @patch(f"{texture_utils_path}._get_max_bin")
    @patch(f"{texture_utils_path}.CropWorkspace")
    @patch(f"{texture_utils_path}.crop_and_rebin")
    def test_crop_wss_and_combine(self, mock_crop_and_rebin, mock_crop, mock_max_bin, mock_rebin, mock_clone, mock_append, mock_sum):
        # inputs
        wss = ["ws1", "ws2"]
        out = "out_ws"
        lower = 1
        upper = 2
        peak = 1.5

        mock_max_bin.return_value = 0.1

        expected_rebin_params = (1, 0.1, 2)

        mock_peak_window_ws = MagicMock()
        mock_peak_window_ws.extractX.return_value = MagicMock()

        _, output_list = crop_wss_and_combine(wss, peak, lower, upper, out)

        # mock returns
        mock_crop.assert_called_once_with(wss[0], lower, upper, OutputWorkspace="__peak_window_crop")
        mock_rebin.assert_called_once_with("__peak_window_crop", expected_rebin_params, OutputWorkspace=f"rebin_ws_{peak}_0")
        mock_clone.assert_called_once_with(InputWorkspace=f"rebin_ws_{peak}_0", OutputWorkspace=f"rebin_ws_{peak}")

        mock_crop_and_rebin.assert_called_once_with(wss[1], f"rebin_ws_{peak}_1", lower, upper, expected_rebin_params)

        mock_append.assert_called_once_with(f"rebin_ws_{peak}", f"rebin_ws_{peak}_1", OutputWorkspace=f"rebin_ws_{peak}")

        mock_sum.assert_called_once_with(f"rebin_ws_{peak}", OutputWorkspace=out)

        self.assertEqual(output_list, [f"rebin_ws_{peak}_0", f"rebin_ws_{peak}_1"])


class TextureUtilsFittingStepsTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}._make_composite")
    @patch(f"{texture_utils_path}._estimate_intensity_background_and_centre")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.crop_wss_and_combine")
    def test_fit_initial_summed_spectra(
        self,
        mock_crop_and_combine,
        mock_func_factory,
        mock_estimate_intens,
        mock_make_comp,
        mock_fit,
    ):
        # inputs
        wss = ["ws1", "ws2"]
        peak1, peak2 = 1.0, 2.0
        peaks = [peak1, peak2]
        peak_window = 0.05
        fit_kwargs = {}
        peak_func_name = "BackToBackExponential"

        # some mock intermediates
        x_vals = [1, 1.5, 2]
        intensities, sigmas, bgs, centres = (2.0, 4.0), (1.0, 1.0), (0.5, 0.5), (1.01, 2.01)

        # some mock returns
        mock_fit.return_value = MagicMock()

        peak1_window_ws, peak2_window_ws = MagicMock(), MagicMock()
        peak1_window_ws.readX.return_value = x_vals
        peak2_window_ws.readX.return_value = x_vals
        peak1_window_ws.name.return_value = "peak_window_0"
        peak2_window_ws.name.return_value = "peak_window_1"

        peak_func1, peak_func2 = MagicMock(), MagicMock()

        comp_func1, comp_func2 = MagicMock(), MagicMock()

        mock_crop_and_combine.side_effect = ((peak1_window_ws, ["ws1_1.0", "ws2_1.0"]), (peak2_window_ws, ["ws1_2.0", "ws2_2.0"]))
        mock_bg_func = MagicMock()
        mock_func_factory.createFunction.return_value = mock_bg_func
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (peak_func1, peak_func2)

        mock_estimate_intens.side_effect = list(zip(intensities, sigmas, bgs, centres))

        mock_make_comp.side_effect = (comp_func1, comp_func2)

        # expected returns
        peak1_kwargs = {"InputWorkspace": "peak_window_0", "StartX": 0.95, "EndX": 1.05}
        peak2_kwargs = {"InputWorkspace": "peak_window_1", "StartX": 1.95, "EndX": 2.05}

        # exec
        _, all_wss = fit_initial_summed_spectra(wss, peaks, peak_window, fit_kwargs, peak_func_name)

        # assert

        mock_estimate_intens.assert_has_calls(
            [
                call(peak1_window_ws, 0, 0, 2, peak1),  # 2 is len(x_val) -1
                call(peak2_window_ws, 0, 0, 2, peak2),
            ]
        )

        peak_func1.setParameter.assert_any_call("X0", centres[0])
        peak_func1.setParameter.assert_any_call("I", intensities[0])
        peak_func2.setParameter.assert_any_call("X0", centres[1])
        peak_func2.setParameter.assert_any_call("I", intensities[1])

        # setMatrixWorkspace should be called on each peak
        peak_func1.setMatrixWorkspace.assert_called_once_with(peak1_window_ws, 0, 0.95, 1.05)
        peak_func2.setMatrixWorkspace.assert_called_once_with(peak2_window_ws, 0, 1.95, 2.05)

        mock_fit.assert_has_calls(
            [
                call(
                    Function=comp_func1,
                    Output=f"composite_fit_{peak1}",
                    MaxIterations=50,
                    **peak1_kwargs,
                ),
                call(
                    Function=comp_func2,
                    Output=f"composite_fit_{peak2}",
                    MaxIterations=50,
                    **peak2_kwargs,
                ),
            ],
            any_order=True,
        )
        self.assertEqual(all_wss, [["ws1_1.0", "ws2_1.0"], ["ws1_2.0", "ws2_2.0"]])

    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}._make_composite")
    @patch(f"{texture_utils_path}._estimate_intensity_background_and_centre")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.crop_wss_and_combine")
    def test_fit_initial_summed_spectra_ikeda_carpenter_fixes_params(
        self,
        mock_crop_and_combine,
        mock_func_factory,
        mock_estimate_intens,
        mock_make_comp,
        mock_fit,
    ):
        wss = ["ws1"]
        peaks = [1.0]
        peak_window = 0.05
        fit_kwargs = {}
        peak_func_name = "IkedaCarpenterPV"

        x_vals = [1, 1.5, 2]
        window_ws = MagicMock()
        window_ws.readX.return_value = x_vals
        window_ws.name.return_value = "peak_window_0"

        mock_crop_and_combine.return_value = (window_ws, ["ws1_1.0"])
        mock_func_factory.createFunction.return_value = MagicMock()
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        peak_func = MagicMock()
        mock_instance.createPeakFunction.return_value = peak_func
        mock_estimate_intens.return_value = (2.0, 1.0, 0.5, 1.01)
        mock_make_comp.return_value = MagicMock()
        mock_fit.return_value = MagicMock()

        fit_initial_summed_spectra(wss, peaks, peak_window, fit_kwargs, peak_func_name)

        # IkedaCarpenterPV should fix these instrument-dependent parameters
        peak_func.fixParameter.assert_has_calls(
            [call("Alpha0"), call("Alpha1"), call("Beta0"), call("Kappa")],
            any_order=False,
        )

    @patch(f"{texture_utils_path}._tie_bkg")
    @patch(f"{texture_utils_path}._make_composite")
    @patch(f"{texture_utils_path}._estimate_intensity_background_and_centre")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.DeltaEModeType")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.MultiDomainFunction")
    def test_get_initial_fit_function_and_kwargs_from_specs(
        self, mock_gen_mdf, mock_func_factory, mock_delta_e, mock_unit_conv, mock_estimate_intens, mock_make_comp, mock_tie_bkg
    ):
        # inputs

        mock_ws = MagicMock()
        mock_ws_tof = MagicMock()
        peak = 1.0
        x_window = (0.95, 1.05)
        x0_window = (0.99, 1.01)
        parameters_to_tie = ("A", "B")
        peak_func_name = "BackToBackExponential"
        bg_func_name = "LinearBackground"
        bkg_is_tied = True

        # mock intermediates

        # mock spectrumInfo
        mock_si = MagicMock()
        mock_si.size.return_value = 2  # two spectra
        diff_consts = MagicMock()
        mock_si.diffractometerConstants.return_value = diff_consts
        mock_ws.spectrumInfo.return_value = mock_si

        # mock functions and function factory
        base_peak_func, peak_func1, peak_func2 = MagicMock(), MagicMock(), MagicMock()

        comp_func1, comp_func2 = MagicMock(), MagicMock()

        mock_func_factory.createFunction.return_value = MagicMock()
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (base_peak_func, peak_func1, peak_func2)

        # mock unit conversion

        tof_peaks, tof_starts, tof_ends = (10, 11), (8, 9), (15, 16)  # not physically accurate TOF but easier to keep track of
        x0_lowers, x0_uppers = (0.99, 0.99), (1.01, 1.01)
        mock_delta_e.Elastic = "elastic"
        unit_conv_res1, unit_conv_res2 = zip(tof_peaks, tof_starts, tof_ends, x0_lowers, x0_uppers)
        mock_unit_conv.run.side_effect = unit_conv_res1 + unit_conv_res2

        # mock ws_tof data access

        istarts, iends = (2, 3), (100, 101)  # mock indices for the indices corresponding to an x value
        mock_ws_tof.yIndexOfX.side_effect = [istarts[0], iends[0], istarts[1], iends[1]]
        mock_ws_tof.name.return_value = "ws_tof"

        # mock estimate

        intensities, sigmas, bgs, x0s = (2.0, 4.0), (1.0, 1.0), (0.0, 0.0), (10, 11)

        mock_estimate_intens.side_effect = list(zip(intensities, sigmas, bgs, x0s))

        # mock mdf

        mock_mdf = MagicMock()
        mock_gen_mdf.return_value = mock_mdf
        mock_mdf.nDomains.return_value = 2

        mock_make_comp.side_effect = (comp_func1, comp_func2)

        # mock bkg tie

        mock_tie_bkg.return_value = (mock_mdf, [])

        # mock func processing

        base_peak_func.nParams.return_value = 5
        base_peak_func.getParamName.side_effect = ("A", "B", "I", "X0", "S", "A", "B", "I", "X0", "S")

        # exec

        out_func, out_kwargs, _ = get_initial_fit_function_and_kwargs_from_specs(
            mock_ws, mock_ws_tof, peak, x_window, x0_window, parameters_to_tie, peak_func_name, bg_func_name, bkg_is_tied
        )

        # assert

        base_peak_func.getWidthParameterName.assert_called_once()

        vals_to_convert = (1.0, 0.95, 1.05, 0.99, 1.01, 1.0, 0.95, 1.05, 0.99, 1.01)
        mock_unit_conv.run.assert_has_calls([call("dSpacing", "TOF", val, 0, mock_delta_e.Elastic, diff_consts) for val in vals_to_convert])

        mock_mdf.add.assert_has_calls([call(comp_func1), call(comp_func2)])
        mock_mdf.setDomainIndex.assert_has_calls([call(0, 0), call(1, 1)])
        mock_mdf.setMatrixWorkspace.assert_has_calls([call(mock_ws_tof, i, tof_starts[i], tof_ends[i]) for i in range(2)])

        mock_mdf.addTies.assert_called_once_with("f1.f0.A=f0.f0.A,f1.f0.B=f0.f0.B")

        # the function object itself is returned, not a string
        self.assertIs(out_func, mock_mdf)

        peak_func1.setParameter.assert_any_call("X0", x0s[0])
        peak_func1.setParameter.assert_any_call("I", intensities[0])
        peak_func1.setMatrixWorkspace.assert_called_once()

        expected_spec_kwargs = {
            "InputWorkspace": "ws_tof",
            "StartX": tof_starts[0],
            "EndX": tof_ends[0],
            "WorkspaceIndex": 0,
            "InputWorkspace_1": "ws_tof",
            "StartX_1": tof_starts[1],
            "EndX_1": tof_ends[1],
            "WorkspaceIndex_1": 1,
        }

        self.assertEqual(expected_spec_kwargs, out_kwargs)

    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}._make_composite")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.MultiDomainFunction")
    def test_rerun_fit_with_new_ws(
        self,
        mock_gen_mdf,
        mock_func_factory,
        mock_make_comp,
        mock_fit,
    ):
        # inputs
        mdf = MagicMock()
        fit_kwargs = {"SomeKwarg": 123}
        md_fit_kwargs = {
            "InputWorkspace": "old_ws",
            "StartX": 10.0,
            "EndX": 20.0,
            "WorkspaceIndex": 0,
            "InputWorkspace_1": "old_ws",
            "StartX_1": 30.0,
            "EndX_1": 40.0,
            "WorkspaceIndex_1": 1,
        }
        new_ws = MagicMock()
        new_ws.name.return_value = "new_ws"

        x0_frac_move = 0.1
        iters = 50
        parameters_to_fix = ("A", "B")
        tie_background = True

        # mock existing mdf domains: two composite functions (peak + bg)
        mdf.nFunctions.return_value = 2

        peak0, peak1 = MagicMock(), MagicMock()
        bg0, bg1 = MagicMock(), MagicMock()

        peak0.name.return_value = "BackToBackExponential"
        peak1.name.return_value = "BackToBackExponential"

        # peak parameter vals
        peak0.getParameterValue.side_effect = lambda p: {"I": 0.5, "X0": 10.0, "A": 1, "B": 1, "S": 1}[p]
        peak1.getParameterValue.side_effect = lambda p: {"I": 4.0, "X0": 20.0, "A": 1, "B": 1, "S": 1}[p]

        # background params
        bg1.nParams.return_value = 2
        bg1.getParamName.side_effect = ("A0", "A1")

        comp0, comp1 = MagicMock(), MagicMock()
        comp0.__getitem__.side_effect = lambda i: [peak0, bg0][i]
        comp1.__getitem__.side_effect = lambda i: [peak1, bg1][i]
        mdf.__getitem__.side_effect = (comp0, comp1)

        # mock mdf
        new_func = MagicMock()
        mock_gen_mdf.return_value = new_func

        # mock func factory
        new_peak0, new_peak1 = MagicMock(), MagicMock()
        new_peak0.name.return_value = "BackToBackExponential"
        new_peak1.name.return_value = "BackToBackExponential"
        # mock nParams and getParamName for the non-IC path
        new_peak0.nParams.return_value = 5
        new_peak0.getParamName.side_effect = lambda i: ["I", "X0", "A", "B", "S"][i]
        new_peak1.nParams.return_value = 5
        new_peak1.getParamName.side_effect = lambda i: ["I", "X0", "A", "B", "S"][i]
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (new_peak0, new_peak1)

        # mock composite functions
        comp_out0, comp_out1 = MagicMock(), MagicMock()
        mock_make_comp.side_effect = (comp_out0, comp_out1)

        # mock fit
        fit_return = MagicMock()
        mock_fit.return_value = fit_return

        # exec
        out_fit, out_md_kwargs = rerun_fit_with_new_ws(
            mdf=mdf,
            fit_kwargs=fit_kwargs,
            md_fit_kwargs=md_fit_kwargs,
            new_ws=new_ws,
            x0_frac_move=x0_frac_move,
            iters=iters,
            parameters_to_fix=parameters_to_fix,
            tie_background=tie_background,
        )

        # assert
        self.assertEqual(out_md_kwargs["InputWorkspace"], "new_ws")
        self.assertEqual(out_md_kwargs["InputWorkspace_1"], "new_ws")

        mock_instance.createPeakFunction.assert_has_calls([call("BackToBackExponential"), call("BackToBackExponential")])

        # constraints updated around new values
        # note intens is max(I, 1)
        # I=0.5 -> max(0.5,1)=1, x0=10 then bounds should be 0.5<I<2 and 9<X0<11
        new_peak0.addConstraints.assert_has_calls(
            [call("0.5<I<2"), call("9.0<X0<11.0")],
        )
        # domain1: I=4 and x0=20 then bounds should be 2.0<I<8.0 and 18<X0<22
        new_peak1.addConstraints.assert_has_calls(
            [call("2.0<I<8.0"), call("18.0<X0<22.0")],
        )

        for p in parameters_to_fix:
            new_peak0.fixParameter.assert_any_call(p)
            new_peak1.fixParameter.assert_any_call(p)

        # now uses _make_composite and adds the composite directly
        new_func.add.assert_has_calls([call(comp_out0), call(comp_out1)])
        new_func.setDomainIndex.assert_has_calls([call(0, 0), call(1, 1)])

        # ties are added via addTies, not string concatenation
        new_func.addTies.assert_called_once_with("f1.f1.A0=f0.f1.A0,f1.f1.A1=f0.f1.A1")

        # Function object is passed directly (not as string)
        mock_fit.assert_called_once_with(
            Function=new_func,
            Output="fit_new_ws",
            MaxIterations=iters,
            **fit_kwargs,
            **out_md_kwargs,
        )

        # setMatrixWorkspace called on each new peak function
        new_peak0.setMatrixWorkspace.assert_called()
        new_peak1.setMatrixWorkspace.assert_called()

        # return values
        self.assertIs(out_fit, fit_return)
        self.assertIs(out_md_kwargs, md_fit_kwargs)

    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}._make_composite")
    @patch(f"{texture_utils_path}._get_default_param_ties")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.MultiDomainFunction")
    def test_rerun_fit_with_new_ws_last_fit_ic(
        self,
        mock_gen_mdf,
        mock_func_factory,
        mock_get_default_ties,
        mock_make_comp,
        mock_fit,
    ):
        mdf = MagicMock()
        fit_kwargs = {"SomeKwarg": 123}
        md_fit_kwargs = {
            "InputWorkspace": "old_ws",
            "StartX": 10.0,
            "EndX": 20.0,
            "WorkspaceIndex": 0,
        }
        new_ws = MagicMock()
        new_ws.name.return_value = "new_ws"

        mdf.nFunctions.return_value = 1

        peak0 = MagicMock()
        bg0 = MagicMock()
        peak0.name.return_value = "BackToBackExponential"
        peak0.getParameterValue.side_effect = lambda p: {"I": 5.0, "X0": 15.0}[p]

        comp0 = MagicMock()
        comp0.__getitem__.side_effect = lambda i: [peak0, bg0][i]
        mdf.__getitem__.side_effect = (comp0,)

        new_func = MagicMock()
        mock_gen_mdf.return_value = new_func

        new_peak = MagicMock()
        new_peak.name.return_value = "IkedaCarpenterPV"
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.return_value = new_peak

        mock_get_default_ties.return_value = ("Alpha0", "Alpha1", "Beta0", "Kappa")

        mock_make_comp.return_value = MagicMock()
        mock_fit.return_value = MagicMock()

        out_fit, _ = rerun_fit_with_new_ws(
            mdf=mdf,
            fit_kwargs=fit_kwargs,
            md_fit_kwargs=md_fit_kwargs,
            new_ws=new_ws,
            x0_frac_move=0.1,
            iters=50,
            is_final=True,
            last_fit_ic=True,
        )

        # should create IkedaCarpenterPV (not BackToBackExponential)
        mock_instance.createPeakFunction.assert_called_once_with("IkedaCarpenterPV")

        # should set X0 and I before setMatrixWorkspace
        new_peak.setParameter.assert_any_call("X0", 15.0)
        new_peak.setParameter.assert_any_call("I", 5.0)
        new_peak.setMatrixWorkspace.assert_called()

        # should get default IC ties since peak was not already IkedaCarpenter
        mock_get_default_ties.assert_called_once_with("IkedaCarpenterPV", None)


class TextureUtilsOverallFittingTests(unittest.TestCase):
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
    @patch(f"{texture_utils_path}.convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.rerun_fit_with_new_ws")
    @patch(f"{texture_utils_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}.Rebunch")
    @patch(f"{texture_utils_path}.fit_initial_summed_spectra")
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
        mock_fit_summed,
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
        mock_fit_summed.return_value = ([(0.95, 1.05)], [["ws_crop_rebin_peak0_ws0"]])

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
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0])

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
            (0.95, 1.05),
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

        expected_kwargs = {
            "StepSizeMethod": "Sqrt epsilon",
            "IgnoreInvalidData": False,
            "CreateOutput": True,
            "OutputCompositeMembers": True,
            "Minimizer": "Levenberg-Marquardt",
            "CostFunction": "Unweighted least squares",
        }

        mock_rerun_with_new.assert_has_calls(
            [
                call("md_function", expected_kwargs, md_fit_kwargs, "smooth_ws_2", 0.02, 50, None, ("A", "B"), True, False, False),
                call("md_function_final", expected_kwargs, md_fit_kwargs, "ws_tof", 0.02, 50, None, ("A", "B"), True, True, False),
            ]
        )
        self.assertEqual(tab_ws.addRow.call_count, num_spec)

    # -------- MULTIPLE wss & peaks --------

    @patch(f"{texture_utils_path}.SaveNexus")
    @patch(f"{texture_utils_path}.CreateEmptyTableWorkspace")
    @patch(
        f"{texture_utils_path}.calc_intens_and_sigma_arrays",
        return_value=(None, None, np.array([3.0, 3.0]), None),
    )
    @patch(f"{texture_utils_path}.convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.rerun_fit_with_new_ws")
    @patch(f"{texture_utils_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}.Rebunch")
    @patch(f"{texture_utils_path}.fit_initial_summed_spectra")
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
        mock_fit_summed,
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
        mock_fit_summed.return_value = ([(0.95, 1.05), (1.95, 2.05)], [["ws1_1.0", "ws2_1.0"], ["ws1_2.0", "ws2_2.0"]])

        wss = ["TEST000101_ws", "TEST000102_ws"]
        runs = ["000101", "000102"]
        prefix, group, peaks, num_spec = "TEST", "TestGroup", [1.0, 2.0], 2

        ws1, ws2 = MagicMock(), MagicMock()
        si1, si2 = MagicMock(), MagicMock()
        si1.size.return_value = num_spec
        si2.size.return_value = num_spec
        ws1.spectrumInfo.return_value = si1
        ws2.spectrumInfo.return_value = si2

        mock_convert_units.side_effect = ["ws1_tof_1", "ws1_tof_2", "ws2_tof_1", "ws2_tof_2"]
        mock_rebunch.side_effect = [
            "smooth_ws_3_1",
            "smooth_ws_2_1",
            "smooth_ws_3_2",
            "smooth_ws_2_2",
            "smooth_ws_3_1",
            "smooth_ws_2_1",
            "smooth_ws_3_2",
            "smooth_ws_2_2",
        ]

        md_fit_kwargs = {"InputWorkspace": "ws_tof", "StartX": 0.9, "EndX": 1.1, "WorkspaceIndex": 0}
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0])

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
        self.assertEqual(mock_rerun_with_new.call_count, expected_calls * 2)
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
                x0_window=(0.95, 1.05),
                parameters_to_tie=["NotAParam"],
                peak_func_name="BackToBackExponential",
                bg_func_name="LinearBackground",
                tie_bkg=False,
            )
        self.assertIn("Invalid parameter(s) to tie", str(ctx.exception))


class TextureUtilsNewFunctionsTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.FunctionFactory")
    def test_make_composite_builds_composite_with_peak_and_bg(self, mock_factory):
        peak_func = MagicMock()
        bg_func = MagicMock()
        comp = MagicMock()
        mock_factory.createFunction.return_value = comp

        result = _make_composite(peak_func, bg_func)

        mock_factory.createFunction.assert_called_once_with("CompositeFunction")
        comp.add.assert_has_calls([call(peak_func), call(bg_func)])
        self.assertIs(result, comp)

    def test_get_default_param_ties_b2b(self):
        result = _get_default_param_ties("BackToBackExponential", None)
        self.assertEqual(result, ("A", "B"))

    def test_get_default_param_ties_ikeda_carpenter(self):
        result = _get_default_param_ties("IkedaCarpenterPV", None)
        self.assertEqual(result, ("Alpha0", "Alpha1", "Beta0", "Kappa"))

    def test_get_default_param_ties_preserves_explicit_ties(self):
        explicit = ("X0", "S")
        result = _get_default_param_ties("BackToBackExponential", explicit)
        self.assertIs(result, explicit)

    def test_get_default_param_ties_unknown_func_returns_none(self):
        result = _get_default_param_ties("Gaussian", None)
        self.assertIsNone(result)

    @patch(f"{texture_utils_path}.calc_sigma_from_summation")
    @patch(f"{texture_utils_path}.get_eval_ws")
    def test_calc_intens_and_sigma_arrays(self, mock_get_eval_ws, mock_calc_sigma):
        # set up a mock fit result with 2 domains
        func = MagicMock()
        comp0, comp1 = MagicMock(), MagicMock()
        comp0.getParameterValue.return_value = 10.0  # I for domain 0
        comp1.getParameterValue.return_value = 20.0  # I for domain 1
        func.__iter__ = MagicMock(return_value=iter([comp0, comp1]))
        func.nDomains.return_value = 2

        # mock workspace data
        ws0, ws1 = MagicMock(), MagicMock()
        ws0.readX.return_value = np.array([1, 2, 3])
        ws0.readE.return_value = np.array([0.1, 0.2, 0.3])
        ws0.readY.return_value = np.array([5, 6, 7])  # readY(3) for calc function
        ws1.readX.return_value = np.array([4, 5, 6])
        ws1.readE.return_value = np.array([0.4, 0.5, 0.6])
        ws1.readY.return_value = np.array([8, 9, 10])
        mock_get_eval_ws.side_effect = [ws0, ws1]

        mock_calc_sigma.side_effect = [(2.0, 1.5), (5.0, 4.0)]

        fit_result = {"Function": func, "OutputWorkspace": "fit_ws"}
        intens, sigma, i_over_sig, peak_limits = calc_intens_and_sigma_arrays(fit_result)

        np.testing.assert_array_equal(intens, [10.0, 20.0])
        np.testing.assert_array_equal(sigma, [2.0, 5.0])
        np.testing.assert_allclose(i_over_sig, [10.0 / 2.0, 20.0 / 5.0])
        np.testing.assert_array_equal(peak_limits, [1.5, 4.0])

    @patch(f"{texture_utils_path}.calc_sigma_from_summation")
    @patch(f"{texture_utils_path}.get_eval_ws")
    def test_calc_intens_and_sigma_arrays_zero_sigma(self, mock_get_eval_ws, mock_calc_sigma):
        func = MagicMock()
        comp0 = MagicMock()
        comp0.getParameterValue.return_value = 10.0
        func.__iter__ = MagicMock(return_value=iter([comp0]))
        func.nDomains.return_value = 1

        ws0 = MagicMock()
        ws0.readX.return_value = np.array([1, 2, 3])
        ws0.readE.return_value = np.array([0.1, 0.2, 0.3])
        ws0.readY.return_value = np.array([5, 6, 7])
        mock_get_eval_ws.return_value = ws0
        mock_calc_sigma.return_value = (0.0, None)

        fit_result = {"Function": func, "OutputWorkspace": "fit_ws"}
        _, _, i_over_sig, _ = calc_intens_and_sigma_arrays(fit_result)

        self.assertEqual(i_over_sig[0], 0.0)


class TextureUtilsFitAllPeaksNewParamsTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.fit_initial_summed_spectra")
    def test_fit_all_peaks_warns_on_unsupported_peak_func(self, mock_fit_summed, mock_logger):
        mock_fit_summed.return_value = ([], [])

        fit_all_peaks(
            wss=[],
            peaks=[],
            peak_window=0.1,
            save_dir="save",
            peak_func_name="Gaussian",
        )

        mock_logger.warning.assert_called_once()
        self.assertIn("Gaussian", mock_logger.warning.call_args[0][0])

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.fit_initial_summed_spectra")
    def test_fit_all_peaks_no_warning_for_supported_peak_funcs(self, mock_fit_summed, mock_logger):
        mock_fit_summed.return_value = ([], [])

        for func_name in ("BackToBackExponential", "IkedaCarpenterPV"):
            mock_logger.reset_mock()
            fit_all_peaks(
                wss=[],
                peaks=[],
                peak_window=0.1,
                save_dir="save",
                peak_func_name=func_name,
            )
            mock_logger.warning.assert_not_called()

    @patch(f"{texture_utils_path}.fit_initial_summed_spectra")
    def test_fit_all_peaks_passes_peak_func_name_to_summed_fit(self, mock_fit_summed):
        mock_fit_summed.return_value = ([], [])

        fit_all_peaks(
            wss=[],
            peaks=[],
            peak_window=0.1,
            save_dir="save",
            peak_func_name="IkedaCarpenterPV",
        )

        _, kwargs = mock_fit_summed.call_args
        args = mock_fit_summed.call_args[0]
        self.assertEqual(args[4], "IkedaCarpenterPV")

    @patch(f"{texture_utils_path}.CloneWorkspace")
    @patch(f"{texture_utils_path}.SaveNexus")
    @patch(f"{texture_utils_path}.CreateEmptyTableWorkspace")
    @patch(
        f"{texture_utils_path}.calc_intens_and_sigma_arrays",
        return_value=(None, None, np.array([3.5, 5.0]), None),
    )
    @patch(f"{texture_utils_path}.convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.rerun_fit_with_new_ws")
    @patch(f"{texture_utils_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}.Rebunch")
    @patch(f"{texture_utils_path}.fit_initial_summed_spectra")
    @patch(f"{texture_utils_path}.ConvertUnits")
    @patch(f"{texture_utils_path}._get_grouping_from_ws_log")
    @patch(f"{texture_utils_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{texture_utils_path}.ADS")
    def test_fit_all_peaks_last_fit_ic_adds_extra_fit_ws(
        self,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_convert_units,
        mock_fit_summed,
        mock_rebunch,
        mock_fit,
        mock_get_initial,
        mock_rerun_with_new,
        mock_unitconv,
        mock_convert_toferr_to_derr,
        _mock_calc_i_over_sig,
        mock_create_tab_ws,
        mock_save_nexus,
        mock_clone_ws,
    ):
        """When last_fit_ic=True with B2B peak func, an extra fit iteration should be added."""
        mock_unitconv.run.return_value = 1.2345
        mock_convert_toferr_to_derr.return_value = 0.01
        mock_fit_summed.return_value = ([(0.95, 1.05)], [["ws_crop_rebin_peak0_ws0"]])

        wsname = "TEST123456_ws"
        num_spec = 2

        ws = MagicMock()
        si = MagicMock()
        si.size.return_value = num_spec
        ws.spectrumInfo.return_value = si

        mock_convert_units.return_value = "ws_tof"
        mock_rebunch.side_effect = ["smooth_ws_3", "smooth_ws_2"]
        mock_clone_ws.return_value = "ws_tof_IkedaCarpenter"

        md_fit_kwargs = {"InputWorkspace": "ws_tof", "StartX": 0.9, "EndX": 1.1, "WorkspaceIndex": 0}
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0])

        fit_obj = MagicMock()
        fit_obj.Function.function = "md_function"
        fit_obj.OutputWorkspace.name.return_value = "fit_out_ws"
        mock_fit.return_value = fit_obj
        mock_rerun_with_new.return_value = (fit_obj, md_fit_kwargs)

        param_ws = MagicMock()
        param_ws.column.side_effect = lambda which: {"Name": ["Cost Function"], "Value": [1.0], "Error": [0.0]}[which]
        mock_ads.retrieve.side_effect = [ws, param_ws]
        mock_get_run_prefix.return_value = ("123456", "TEST")
        mock_get_group.return_value = "TestGroup"
        mock_create_tab_ws.return_value = MagicMock()

        fit_all_peaks(
            wss=[wsname],
            peaks=[1.0],
            peak_window=0.1,
            save_dir="save",
            override_dir=True,
            last_fit_ic=True,
            peak_func_name="BackToBackExponential",
        )

        # with smooth_vals=(3,2) + final_fit_raw + last_fit_ic, there should be 4 rerun calls
        # (smooth_3 -> smooth_2 -> raw -> IC clone)
        self.assertEqual(mock_rerun_with_new.call_count, 3)

        # The last call should pass last_fit_ic=True and is_final=True
        last_call_args = mock_rerun_with_new.call_args_list[-1][0]
        self.assertTrue(last_call_args[-1])  # last_fit_ic
        self.assertTrue(last_call_args[-2])  # is_final


class TextureUtilsPoleFigureTests(unittest.TestCase):
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
