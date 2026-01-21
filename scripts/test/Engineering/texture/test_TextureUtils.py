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
    fit_initial_summed_spectra,
    rerun_fit_with_new_ws,
    _tie_bkg,
    _get_rebin_params_for_workspaces,
    _rebin_and_rebunch,
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


class TextureUtilsFocusTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.mk")
    @patch(f"{texture_utils_path}.TextureInstrument")
    def test_run_focus_script_instantiates_model_and_calls_main(self, mock_instr, mock_mk):
        mock_model = MagicMock()
        mock_instr.return_value = mock_model
        run_focus_script(wss=["1", "2"], focus_dir="focus", van_run="v", ceria_run="c", full_instr_calib="f", grouping="1")
        mock_instr.assert_called_once()
        mock_model.main.assert_called_once()
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

    @patch(f"{texture_utils_path}.ADS")
    def test_get_rebin_params_for_workspaces(self, mock_ads):
        ws1, ws2 = MagicMock(), MagicMock()
        ws1.getNumberHistograms.return_value = 2
        ws2.getNumberHistograms.return_value = 1

        # ws1 spectra:
        # spec0: min=0, max=18, step=2
        # spec1: min=1, max=49, step=1
        ws1.readX.side_effect = [
            np.arange(0, 20, 2),
            np.arange(1, 50, 1),
        ]

        # ws2 spec0: min=1, max=38, max step=2
        ws2.readX.side_effect = [np.arange(1, 40, 2)]

        mock_ads.retrieve.side_effect = (ws1, ws2)

        out = _get_rebin_params_for_workspaces(["ws1", "ws2"])

        # assert
        mock_ads.retrieve.assert_has_calls([call("ws1"), call("ws2")])

        self.assertEqual(out, "1, 2, 18")

    @patch(f"{texture_utils_path}.Rebunch")
    @patch(f"{texture_utils_path}.Rebin")
    @patch(f"{texture_utils_path}._get_rebin_params_for_workspaces")
    def test__rebin_and_rebunch(self, mock_get_rebin, mock_rebin, mock_rebunch):
        # inputs
        ws = "ws1"
        val = 5

        # mock returns
        mock_get_rebin.return_value = "0.5,0.01,2.5"
        rebunched = MagicMock()
        mock_rebunch.return_value = rebunched

        # exec
        out = _rebin_and_rebunch(ws, val)

        # assert
        mock_get_rebin.assert_called_once_with((ws,))

        mock_rebin.assert_called_once_with(
            InputWorkspace=ws,
            OutputWorkspace="rebin_ws1",
            Params="0.5,0.01,2.5",
        )

        mock_rebunch.assert_called_once_with(
            InputWorkspace="rebin_ws1",
            OutputWorkspace="smooth_ws_5",
            NBunch=5,
        )

        self.assertIs(out, rebunched)


class TextureUtilsFittingStepsTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}.CompositeFunctionWrapper")
    @patch(f"{texture_utils_path}._estimate_intensity_background_and_centre")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.CropWorkspace")
    @patch(f"{texture_utils_path}.SumSpectra")
    @patch(f"{texture_utils_path}.AppendSpectra")
    @patch(f"{texture_utils_path}.CloneWorkspace")
    @patch(f"{texture_utils_path}.Rebin")
    @patch(f"{texture_utils_path}._get_rebin_params_for_workspaces")
    def test_fit_initial_summed_spectra(
        self,
        mock_get_rebin_params,
        mock_rebin,
        mock_clone,
        mock_append,
        mock_sum,
        mock_crop,
        mock_func_factory,
        mock_estimate_intens,
        mock_comp,
        mock_fit,
    ):
        # inputs
        wss = ["ws1", "ws2"]
        peak1, peak2 = 1.0, 2.0
        peaks = [peak1, peak2]
        peak_window = 0.05
        fit_kwargs = {}

        # some mock intermediates
        rebin_params = "0.5,0.01,2.5"
        x_vals = [1, 1.5, 2]
        intensities, sigmas = (2.0, 4.0), (1.0, 1.0)

        # some mock returns
        mock_fit.return_value = MagicMock()
        mock_get_rebin_params.return_value = rebin_params

        peak1_window_ws, peak2_window_ws = MagicMock(), MagicMock()
        peak1_window_ws.readX.return_value = x_vals
        peak2_window_ws.readX.return_value = x_vals
        peak1_window_ws.name.return_value = "peak_window_0"
        peak2_window_ws.name.return_value = "peak_window_1"

        peak_func1, peak_func2 = MagicMock(), MagicMock()

        comp_func1, comp_func2 = MagicMock(), MagicMock()

        mock_crop.side_effect = (peak1_window_ws, peak2_window_ws)
        mock_func_factory.createFunction.return_value = MagicMock()
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (peak_func1, peak_func2)

        mock_estimate_intens.side_effect = list(zip(intensities, sigmas))

        mock_comp.side_effect = (comp_func1, comp_func2)

        # expected returns
        peak1_kwargs = {"InputWorkspace": "peak_window_0", "StartX": 0.95, "EndX": 1.05}
        peak2_kwargs = {"InputWorkspace": "peak_window_1", "StartX": 1.95, "EndX": 2.05}

        # exec
        fit_initial_summed_spectra(wss, peaks, peak_window, fit_kwargs)

        # assert
        mock_get_rebin_params.assert_called_once_with(wss)
        mock_rebin.assert_has_calls(
            [call("ws1", rebin_params, OutputWorkspace="rebin_0"), call("ws2", rebin_params, OutputWorkspace="rebin_1")]
        )

        mock_sum.assert_has_calls(
            [
                call("rebin_0", OutputWorkspace="sum_0"),
                call("rebin_1", OutputWorkspace="sum_1"),
                call("sum_runs_ws", OutputWorkspace="sum_ws"),
            ]
        )
        mock_clone.assert_called_once_with("sum_0", OutputWorkspace="sum_runs_ws")
        mock_append.assert_called_once_with("sum_runs_ws", "sum_1", OutputWorkspace="sum_runs_ws")
        mock_crop.assert_has_calls(
            [call("sum_ws", 0.95, 1.05, OutputWorkspace="peak_window_0"), call("sum_ws", 1.95, 2.05, OutputWorkspace="peak_window_1")]
        )
        mock_estimate_intens.assert_has_calls(
            [
                call(peak1_window_ws, 0, 0, 2, peak1),  # 2 is len(x_val) -1
                call(peak2_window_ws, 0, 0, 2, peak2),
            ]
        )

        peak_func1.setCentre.assert_called_once_with(peak1)
        peak_func2.setCentre.assert_called_once_with(peak2)

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

    @patch(f"{texture_utils_path}._tie_bkg")
    @patch(f"{texture_utils_path}.CompositeFunctionWrapper")
    @patch(f"{texture_utils_path}._estimate_intensity_background_and_centre")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.DeltaEModeType")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.MultiDomainFunction")
    def test_get_initial_fit_function_and_kwargs_from_specs(
        self, mock_gen_mdf, mock_func_factory, mock_delta_e, mock_unit_conv, mock_estimate_intens, mock_comp, mock_tie_bkg
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
        comp_func1.function = "cf1"
        comp_func2.function = "cf2"

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

        mock_comp.side_effect = (comp_func1, comp_func2)

        # mock bkg tie

        mock_tie_bkg.return_value = (mock_mdf, [])

        # mock func processing

        base_peak_func.nParams.return_value = 5
        base_peak_func.getParamName.side_effect = ("A", "B", "I", "X0", "S", "A", "B", "I", "X0", "S")

        mock_mdf.__str__.return_value = "func"

        # exec

        out_func, out_kwargs, _ = get_initial_fit_function_and_kwargs_from_specs(
            mock_ws, mock_ws_tof, peak, x_window, x0_window, parameters_to_tie, peak_func_name, bg_func_name, bkg_is_tied
        )

        # assert

        base_peak_func.setIntensity.assert_called_once_with(1.0)
        base_peak_func.getCentreParameterName.assert_called_once()
        base_peak_func.getWidthParameterName.assert_called_once()

        vals_to_convert = (1.0, 0.95, 1.05, 0.99, 1.01, 1.0, 0.95, 1.05, 0.99, 1.01)
        mock_unit_conv.run.assert_has_calls([call("dSpacing", "TOF", val, 0, mock_delta_e.Elastic, diff_consts) for val in vals_to_convert])

        mock_mdf.add.assert_has_calls([call(comp_func1.function), call(comp_func2.function)])
        mock_mdf.setDomainIndex.assert_has_calls([call(0, 0), call(1, 1)])
        mock_mdf.setMatrixWorkspace.assert_has_calls([call(mock_ws_tof, i, tof_starts[i], tof_ends[i]) for i in range(2)])

        self.assertEqual(out_func, "func;ties=(f1.f0.A=f0.f0.A,f1.f0.B=f0.f0.B)")

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
    @patch(f"{texture_utils_path}.CompositeFunctionWrapper")
    @patch(f"{texture_utils_path}.FunctionWrapper")
    @patch(f"{texture_utils_path}.FunctionFactory")
    @patch(f"{texture_utils_path}.MultiDomainFunction")
    def test_rerun_fit_with_new_ws(
        self,
        mock_gen_mdf,
        mock_func_factory,
        mock_func_wrapper,
        mock_comp_wrapper,
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
        peak0.getParameterValue.side_effect = lambda p: {"I": 0.5, "X0": 10.0, "A": 1.0, "B": 2.0, "S": 3.0}[p]
        peak1.getParameterValue.side_effect = lambda p: {"I": 4.0, "X0": 20.0, "A": 4.0, "B": 5.0, "S": 6.0}[p]

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
        new_func.__str__.return_value = "newfunc"

        # mock func factory
        new_peak0, new_peak1 = MagicMock(), MagicMock()
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (new_peak0, new_peak1)

        # mock composite functions
        comp_wrap0, comp_wrap1 = MagicMock(), MagicMock()
        comp_wrap0.function = "cf0"
        comp_wrap1.function = "cf1"
        mock_comp_wrapper.side_effect = (comp_wrap0, comp_wrap1)

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

        expected_set_calls_peak0 = [
            call("I", 0.5),
            call("X0", 10.0),
            call("A", 1.0),
            call("B", 2.0),
            call("S", 3.0),
        ]
        expected_set_calls_peak1 = [
            call("I", 4.0),
            call("X0", 20.0),
            call("A", 4.0),
            call("B", 5.0),
            call("S", 6.0),
        ]
        new_peak0.setParameter.assert_has_calls(expected_set_calls_peak0, any_order=False)
        new_peak1.setParameter.assert_has_calls(expected_set_calls_peak1, any_order=False)

        # constraints updated around new values
        # note intens is max(I, 1)
        # I=0.5 and x0=10 then bounds should be 0.5<I<2 and 9<X0<11
        new_peak0.addConstraints.assert_has_calls(
            [call("0.5<I<2"), call("9.0<X0<11.0")],
        )
        # domain1: I=4 and x0=20 then bounds should be 2<I<8 and 18<X0<22
        new_peak1.addConstraints.assert_has_calls(
            [call("2.0<I<8.0"), call("18.0<X0<22.0")],
        )

        for p in parameters_to_fix:
            new_peak0.fixParameter.assert_any_call(p)
            new_peak1.fixParameter.assert_any_call(p)

        expected_func_str = "newfunc;ties=(f1.f1.A0=f0.f1.A0,f1.f1.A1=f0.f1.A1)"
        new_func.add.assert_has_calls([call("cf0"), call("cf1")])
        new_func.setDomainIndex.assert_has_calls([call(0, 0), call(1, 1)])
        new_func.setMatrixWorkspace.assert_has_calls(
            [
                call(new_ws, 0, out_md_kwargs["StartX"], out_md_kwargs["EndX"]),
                call(new_ws, 1, out_md_kwargs["StartX_1"], out_md_kwargs["EndX_1"]),
            ]
        )

        mock_fit.assert_called_once_with(
            Function=expected_func_str,
            Output="fit_new_ws",
            MaxIterations=iters,
            **fit_kwargs,
            **out_md_kwargs,
        )

        # return values
        self.assertIs(out_fit, fit_return)
        self.assertIs(out_md_kwargs, md_fit_kwargs)


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
    @patch(f"{texture_utils_path}._convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.rerun_fit_with_new_ws")
    @patch(f"{texture_utils_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{texture_utils_path}.Fit")
    @patch(f"{texture_utils_path}._rebin_and_rebunch")
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
        mock_fit_summed.return_value = [
            (0.95, 1.05),
        ]

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
    @patch(f"{texture_utils_path}._rebin_and_rebunch")
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
        mock_fit_summed.return_value = [(0.95, 1.05), (1.95, 2.05)]

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
                x0_window=(0.95, 1.05),
                parameters_to_tie=["NotAParam"],
                peak_func_name="BackToBackExponential",
                bg_func_name="LinearBackground",
                tie_bkg=False,
            )
        self.assertIn("Invalid parameter(s) to tie", str(ctx.exception))


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
