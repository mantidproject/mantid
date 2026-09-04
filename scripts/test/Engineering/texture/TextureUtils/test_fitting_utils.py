# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
# Tests for the shared fitting utilities and the fit_all_peaks orchestration/dispatch layer.
# The per-engine behaviour lives in test_fitpeaks_engine.py.
import unittest
from unittest.mock import patch, MagicMock, call
from os import path
import numpy as np
from Engineering.texture.TextureUtils.fitting_utils import (
    fit_initial_summed_spectra,
    _tie_bkg,
    crop_and_rebin,
    crop_wss_and_combine,
    _make_composite,
    _get_run_and_prefix_from_ws_log,
    _get_grouping_from_ws_log,
    fit_all_peaks,
    _fit_parameters_path,
)

texture_utils_path = "Engineering.texture.TextureUtils.fitting_utils"
fitpeaks_path = "Engineering.texture.TextureUtils.fitpeaks_engine"


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
    @patch(f"{texture_utils_path}.CropWorkspaceRagged")
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
    @patch(f"{texture_utils_path}._get_min_bin")
    @patch(f"{texture_utils_path}.CropWorkspaceRagged")
    @patch(f"{texture_utils_path}.crop_and_rebin")
    def test_crop_wss_and_combine(self, mock_crop_and_rebin, mock_crop, mock_min_bin, mock_rebin, mock_clone, mock_append, mock_sum):
        # inputs
        wss = ["ws1", "ws2"]
        out = "out_ws"
        lower = 1
        upper = 2
        peak = 1.5

        mock_min_bin.return_value = 0.1

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


class TextureUtilsSummedSpectraTests(unittest.TestCase):
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
        peak1_window_ws.x.return_value = x_vals
        peak2_window_ws.x.return_value = x_vals
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
        window_ws.x.return_value = x_vals
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

    def test_fit_initial_summed_spectra_returns_centre_limits_and_crop_wss(self):
        # exercised via a light patch set: the fitted centre becomes the narrow x0 limits, and the
        # per-workspace crop names come straight from crop_wss_and_combine
        with (
            patch(f"{texture_utils_path}.Fit") as mock_fit,
            patch(f"{texture_utils_path}._make_composite"),
            patch(f"{texture_utils_path}._estimate_intensity_background_and_centre", return_value=(2.0, 1.0, 0.5, 1.01)),
            patch(f"{texture_utils_path}.FunctionFactory") as mock_func_factory,
            patch(f"{texture_utils_path}.crop_wss_and_combine") as mock_crop_and_combine,
        ):
            window_ws = MagicMock()
            window_ws.x.return_value = [1, 1.5, 2]
            window_ws.name.return_value = "peak_window_0"
            mock_crop_and_combine.return_value = (window_ws, ["ws1_1.0"])
            mock_func_factory.createFunction.return_value = MagicMock()
            mock_instance = MagicMock()
            mock_func_factory.Instance.return_value = mock_instance
            mock_instance.createPeakFunction.return_value = MagicMock()

            out_peak_func = mock_fit.return_value.Function.function.getFunction.return_value
            out_peak_func.getParameterValue.side_effect = lambda k: {"X0": 1.0}[k]

            x0_lims, all_wss = fit_initial_summed_spectra(["ws1"], [1.0], 0.05, {}, "BackToBackExponential")

        self.assertEqual(len(x0_lims), 1)
        np.testing.assert_allclose(x0_lims[0], (1.0 * (1 - 3e-3), 1.0 * (1 + 3e-3)))
        self.assertEqual(all_wss, [["ws1_1.0"]])


class TextureUtilsSharedHelperTests(unittest.TestCase):
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

    def test_fit_parameters_path_override_dir_is_flat(self):
        result = _fit_parameters_path("save", True, "GROUP", 1.5, "out.nxs")
        self.assertEqual(result, path.join("save", "out.nxs"))

    def test_fit_parameters_path_nested_by_group_and_peak(self):
        result = _fit_parameters_path("save", False, "GROUP", 1.5, "out.nxs")
        self.assertEqual(result, path.join("save", "GROUP", "1.5", "out.nxs"))


class FitAllPeaksOrchestrationTests(unittest.TestCase):
    @patch(f"{fitpeaks_path}._fit_all_peaks_fitpeaks")
    @patch(f"{texture_utils_path}.fit_initial_summed_spectra", return_value=([], []))
    def test_fit_all_peaks_dispatches_to_fitpeaks_by_default(self, _mock_summed, mock_fitpeaks_path):
        # default engine is "fitpeaks": the engine is called with the full positional argument list,
        # so a reordering of e.g. nan_replacement and no_fit_value_dict is caught here
        fit_all_peaks(wss=["ws"], peaks=[1.0], peak_window=0.1, save_dir="save", nan_replacement="min", smooth_vals=(4, 2))
        mock_fitpeaks_path.assert_called_once_with(
            ["ws"],  # wss
            [1.0],  # peaks
            0.1,  # peak_window
            "save",  # save_dir
            False,  # override_dir
            2.0,  # i_over_sigma_thresh
            "min",  # nan_replacement
            None,  # no_fit_value_dict
            "BackToBackExponential",  # peak_func_name
            50,  # max_fit_iters
            {
                "StepSizeMethod": "Sqrt epsilon",
                "IgnoreInvalidData": False,
                "CreateOutput": True,
                "OutputCompositeMembers": True,
                "Minimizer": "Levenberg-Marquardt",
                "CostFunction": "Unweighted least squares",
            },  # fit_kwargs
            (4, 2),  # smooth_vals
            False,  # last_fit_ic
        )

    @patch(f"{texture_utils_path}.fit_initial_summed_spectra", return_value=([], []))
    def test_fit_all_peaks_unknown_engine_raises(self, _mock_summed):
        with self.assertRaises(ValueError):
            fit_all_peaks(wss=["ws"], peaks=[1.0], peak_window=0.1, save_dir="save", engine="bogus")


if __name__ == "__main__":
    unittest.main()
