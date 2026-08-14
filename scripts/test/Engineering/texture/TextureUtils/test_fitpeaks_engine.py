# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from contextlib import ExitStack
from os import path
from types import SimpleNamespace
from unittest.mock import patch, MagicMock, call
import numpy as np
from Engineering.texture.TextureUtils.fitpeaks_engine import (
    _fit_all_peaks_fitpeaks,
    _populate_fitpeaks_output_table,
    _combine_peak_crops_to_tof,
    _compute_tof_windows,
    _estimate_peak_intensities,
    _refine_centre_seed,
    _build_seed_table,
)

texture_utils_path = "Engineering.texture.TextureUtils.fitting_utils"
fitpeaks_path = "Engineering.texture.TextureUtils.fitpeaks_engine"


def _make_peak_func_mock(param_names):
    """A FunctionFactory peak-function mock whose parameter names are param_names."""
    base_peak = MagicMock()
    base_peak.nParams.return_value = len(param_names)
    base_peak.getParamName.side_effect = lambda i: list(param_names)[i]
    return base_peak


def _make_fit_tables(chi2, i_col, x0):
    """A (param_table, error_table) pair mimicking the FitPeaks output tables for len(chi2) rows."""
    n = len(chi2)
    param_table, error_table = MagicMock(), MagicMock()
    param_table.column.side_effect = lambda p: {"chi2": chi2, "I": i_col, "X0": x0}.get(p, [0.0] * n)
    error_table.column.side_effect = lambda p: [0.1] * n
    return param_table, error_table


def _n_output_tables(mock_create_tab):
    """Number of CreateEmptyTableWorkspace calls that made a reported output table.  The same
    algorithm also builds a per-pass seed table, so the raw call count is not the table count."""
    return sum(1 for c in mock_create_tab.call_args_list if c.kwargs["OutputWorkspace"].endswith("_Fit_Parameters"))


def _ads_retrieve(ws, param_table, error_table, ws_names):
    """ADS.retrieve side_effect: raw workspaces by name, else the params/errors table by name."""

    def _retrieve(name):
        if name in ws_names:
            return ws
        return param_table if "params" in name else error_table

    return _retrieve


class FitPeaksOutputTableTests(unittest.TestCase):
    def test_populate_fitpeaks_output_table_columns_and_rows(self):
        out_tab = MagicMock()
        peak_param_names = ["I", "A", "X0"]
        # spectrum 0 fitted, spectrum 1 masked out - pre-sliced (de-interleaved) arrays for one ws
        param_slices = {"I": np.array([10.0, 0.0]), "A": np.array([1.0, 0.0]), "X0": np.array([2.0, 0.0])}
        err_slices = {"I": np.array([1.0, 0.0]), "A": np.array([0.1, 0.0]), "X0": np.array([0.02, 0.0])}
        i_est_vals = np.array([10.0, 0.0])
        fit_mask = np.array([True, False])

        _populate_fitpeaks_output_table(
            out_tab, 2, peak_param_names, param_slices, err_slices, i_est_vals, fit_mask, no_fit_value_dict=None, nan_replacement=None
        )

        # columns: wsindex, I_est, then triple per param
        out_tab.addColumn.assert_any_call("int", "wsindex")
        out_tab.addColumn.assert_any_call("double", "I_est")
        for p in peak_param_names:
            out_tab.addColumn.assert_any_call("double", p)
            out_tab.addColumn.assert_any_call("double", f"{p}_err")
            out_tab.addColumn.assert_any_call("double", f"{p}/{p}_err")
        # one row per spectrum
        self.assertEqual(out_tab.addRow.call_count, 2)
        # fitted spectrum 0: [wsindex, I_est, I, I_err, I/I_err, A, A_err, A/A_err, X0, X0_err, X0/X0_err]
        row0 = out_tab.addRow.call_args_list[0][0][0]
        self.assertEqual(row0[0], 0)
        self.assertEqual(row0[2], 10.0)  # I value
        self.assertEqual(row0[3], 1.0)  # I err
        self.assertAlmostEqual(row0[4], 10.0)  # I / I_err

    def test_populate_fitpeaks_output_table_guards_undetermined_errors(self):
        # a genuinely singular fit can still return a nan/inf/zero parameter error; the value/error
        # ratio must not become nan or inf
        out_tab = MagicMock()
        peak_param_names = ["I", "X0"]
        param_slices = {"I": np.array([10.0, 20.0, 30.0]), "X0": np.array([2.0, 2.0, 2.0])}
        err_slices = {"I": np.array([1.0, 1.0, 1.0]), "X0": np.array([np.nan, 0.0, np.inf])}
        i_est_vals = np.array([10.0, 20.0, 30.0])
        fit_mask = np.array([True, True, True])

        _populate_fitpeaks_output_table(
            out_tab, 3, peak_param_names, param_slices, err_slices, i_est_vals, fit_mask, no_fit_value_dict=None, nan_replacement=None
        )

        # X0 columns are the last triple: [.., X0, X0_err, X0/X0_err]
        for irow in range(3):
            row = out_tab.addRow.call_args_list[irow][0][0]
            x0_err, x0_ratio = row[-2], row[-1]
            self.assertTrue(np.isfinite(x0_ratio))  # never nan/inf
            self.assertEqual(x0_ratio, 0.0)  # undetermined error -> zero ratio
            self.assertEqual(x0_err, np.inf)  # reported as infinite error, matching the unfit convention
            # the valid I error still gives a normal ratio
            self.assertAlmostEqual(row[4], i_est_vals[irow] / 1.0)

    @patch(f"{fitpeaks_path}.replace_nans")
    def test_populate_fitpeaks_output_table_applies_nan_replacement(self, mock_replace):
        # when nan_replacement is set, the assembled table is passed through replace_nans and the
        # replaced values (not the raw ones) are what get written to the output rows
        out_tab = MagicMock()
        peak_param_names = ["I"]
        param_slices = {"I": np.array([10.0, 0.0])}
        err_slices = {"I": np.array([1.0, 0.0])}
        i_est_vals = np.array([10.0, 0.0])
        fit_mask = np.array([True, False])

        # 2 rows x (3 * 1 param + 1) = 4 columns
        replaced = np.array([[1.0, 2.0, 3.0, 4.0], [5.0, 6.0, 7.0, 8.0]])
        mock_replace.return_value = replaced

        _populate_fitpeaks_output_table(
            out_tab, 2, peak_param_names, param_slices, err_slices, i_est_vals, fit_mask, no_fit_value_dict=None, nan_replacement="mean"
        )

        mock_replace.assert_called_once()
        self.assertEqual(mock_replace.call_args[0][1], "mean")  # method forwarded through
        # rows written come from the replace_nans output, prefixed with the spectrum index
        out_tab.addRow.assert_any_call([0] + list(replaced[0]))
        out_tab.addRow.assert_any_call([1] + list(replaced[1]))


class FitPeaksCombineTests(unittest.TestCase):
    @patch(f"{fitpeaks_path}.ConvertUnits")
    @patch(f"{fitpeaks_path}.AppendSpectra")
    @patch(f"{fitpeaks_path}.CloneWorkspace")
    def test_combine_peak_crops_to_tof_appends_every_extra_ws(self, mock_clone, mock_append, mock_convert):
        result = _combine_peak_crops_to_tof(["a", "b", "c"], "combined")

        # first crop clones the combined ws, each remaining crop is appended in order
        mock_clone.assert_called_once_with(InputWorkspace="a", OutputWorkspace="combined")
        mock_append.assert_has_calls(
            [
                call("combined", "b", OutputWorkspace="combined"),
                call("combined", "c", OutputWorkspace="combined"),
            ]
        )
        self.assertEqual(mock_append.call_count, 2)
        # the combined d-spacing ws is converted to TOF and returned
        mock_convert.assert_called_once_with(InputWorkspace="combined", OutputWorkspace="combined", Target="TOF")
        self.assertIs(result, mock_convert.return_value)

    @patch(f"{fitpeaks_path}.ConvertUnits")
    @patch(f"{fitpeaks_path}.AppendSpectra")
    @patch(f"{fitpeaks_path}.CloneWorkspace")
    def test_combine_peak_crops_to_tof_single_ws_has_no_append(self, mock_clone, mock_append, mock_convert):
        _combine_peak_crops_to_tof(["only"], "combined")

        mock_clone.assert_called_once_with(InputWorkspace="only", OutputWorkspace="combined")
        mock_append.assert_not_called()
        mock_convert.assert_called_once_with(InputWorkspace="combined", OutputWorkspace="combined", Target="TOF")


class FitPeaksTofWindowTests(unittest.TestCase):
    @patch(f"{fitpeaks_path}._d_to_tof")
    def test_compute_tof_windows_maps_each_spectrum_independently(self, mock_d_to_tof):
        # every detector maps the same d-spacing centre/window to a different TOF via its own constants
        si = MagicMock()
        si.size.return_value = 2
        dc0, dc1 = MagicMock(), MagicMock()
        si.diffractometerConstants.side_effect = lambda k: [dc0, dc1][k]
        mock_d_to_tof.side_effect = lambda value, dc: value * (10 if dc is dc0 else 20)

        tof_centre, tof_lo, tof_hi = _compute_tof_windows(si, centre=2.0, xmin=1.5, xmax=2.5)

        np.testing.assert_array_equal(tof_centre, [20.0, 40.0])
        np.testing.assert_array_equal(tof_lo, [15.0, 30.0])
        np.testing.assert_array_equal(tof_hi, [25.0, 50.0])


class FitPeaksIntensityEstimateTests(unittest.TestCase):
    @patch(f"{fitpeaks_path}.DeleteWorkspace")
    @patch(f"{fitpeaks_path}.ADS")
    @patch(f"{fitpeaks_path}.EstimatePeakIntensities")
    def test_estimate_peak_intensities_reads_intensity_column(self, mock_alg, mock_ads, mock_delete):
        # delegates to the C++ EstimatePeakIntensities over the shared per-spectrum window workspace,
        # returning its Intensity column (already in spectrum order for a single peak)
        ws = MagicMock()
        windows_ws = "__windows"
        table = MagicMock()
        table.column.return_value = [11.0, 22.0]
        mock_alg.return_value = table
        mock_ads.doesExist.return_value = True

        i_est = _estimate_peak_intensities(ws, windows_ws)

        np.testing.assert_array_equal(i_est, np.array([11.0, 22.0]))
        table.column.assert_called_once_with("Intensity")
        # the input workspace and the shared window workspace are passed straight through
        kwargs = mock_alg.call_args.kwargs
        self.assertIs(kwargs["InputWorkspace"], ws)
        self.assertEqual(kwargs["PeakWindowWorkspace"], windows_ws)
        # the temporary result table is cleaned up
        mock_delete.assert_called_once_with("__fitpeaks_i_est_table")

    @patch(f"{fitpeaks_path}.DeleteWorkspace")
    @patch(f"{fitpeaks_path}.ADS")
    @patch(f"{fitpeaks_path}.EstimatePeakIntensities")
    def test_estimate_peak_intensities_length_follows_intensity_column(self, mock_alg, mock_ads, _mock_delete):
        # the returned array has one entry per spectrum (length of the Intensity column)
        ws = MagicMock()
        table = MagicMock()
        table.column.return_value = [7.0, 7.0, 7.0, 7.0]
        mock_alg.return_value = table
        mock_ads.doesExist.return_value = False

        i_est = _estimate_peak_intensities(ws, "__windows")

        self.assertEqual(i_est.shape, (4,))
        np.testing.assert_array_equal(i_est, np.full(4, 7.0))


# every external call the engine makes, patched for the whole FitPeaksEngineTests class - the engine
# only orchestrates algorithms, so each test drives it through this one shared stack rather than
# repeating an 18-deep decorator tower
_ENGINE_PATCHES = (
    ("ads", "ADS", {}),
    ("run_prefix", "_get_run_and_prefix_from_ws_log", {"return_value": ("123456", "TEST")}),
    ("grouping", "_get_grouping_from_ws_log", {"return_value": "TestGroup"}),
    ("fit_summed", "fit_initial_summed_spectra", {}),
    ("func_factory", "FunctionFactory", {}),
    ("clone", "CloneWorkspace", {}),
    ("append", "AppendSpectra", {}),
    ("convert_units", "ConvertUnits", {}),
    ("fitpeaks", "FitPeaks", {}),
    ("rebunch", "Rebunch", {}),
    ("create_ws", "CreateWorkspace", {}),
    ("delete_ws", "DeleteWorkspace", {}),
    ("makedirs", "makedirs", {}),
    ("toferr", "convert_TOFerror_to_derror", {"return_value": 0.001}),
    ("populate", "_populate_fitpeaks_output_table", {}),
    ("create_tab", "CreateEmptyTableWorkspace", {}),
    ("save_nexus", "SaveNexus", {}),
    ("estimate", "_estimate_peak_intensities", {"side_effect": lambda ws, windows_ws: np.zeros(ws.spectrumInfo().size())}),
)


class FitPeaksEngineTests(unittest.TestCase):
    def setUp(self):
        stack = ExitStack()
        self.addCleanup(stack.close)
        mocks = {name: stack.enter_context(patch(f"{fitpeaks_path}.{target}", **kwargs)) for name, target, kwargs in _ENGINE_PATCHES}
        # the d <-> TOF conversion lives in fitting_utils and is reached through the shared helpers
        mocks["unitconv"] = stack.enter_context(patch(f"{texture_utils_path}.UnitConversion"))
        stack.enter_context(patch(f"{texture_utils_path}.DeltaEModeType"))
        mocks["unitconv"].run.return_value = 5000.0  # any TOF/d value; CreateWorkspace/FitPeaks are mocked
        mocks["create_tab"].return_value = MagicMock()
        self.mocks = SimpleNamespace(**mocks)

    def _setup_combined_ws(self, n_spec_total):
        """The combined workspace the engine gets back from ConvertUnits->TOF."""
        combined_tof = MagicMock()
        combined_tof.spectrumInfo.return_value.size.return_value = n_spec_total
        self.mocks.convert_units.return_value = combined_tof
        return combined_tof

    def _setup_fit_tables(self, param_table, error_table, ws_names):
        """Route ADS.retrieve to the raw workspaces and the FitPeaks result tables."""
        self.mocks.ads.retrieve.side_effect = _ads_retrieve(MagicMock(), param_table, error_table, ws_names)

    def test_fit_all_peaks_fitpeaks_tof_with_smoothing_fallback(self):
        # two workspaces, two spectra each -> combined TOF workspace of 4 spectra for the peak.
        # summed fit returns a 2-tuple (no shape seeds - FitPeaks loads A,B from the instrument)
        self.mocks.fit_summed.return_value = (
            [(0.95, 1.05)],
            [["crop_ws0", "crop_ws1"]],  # per-workspace cropped+rebinned ws names for peak 0
        )
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        combined_tof = self._setup_combined_ws(4)

        # 4 rows: ws0 spectra (0,1) then ws1 spectra (2,3); ws1 spec 3 rejected (chi2=DBL_MAX, I=0)
        param_table, error_table = _make_fit_tables([1.0, 1.0, 1.0, 1e308], [5.0, 6.0, 7.0, 0.0], [5000.0, 5000.0, 5000.0, 0.0])
        self._setup_fit_tables(param_table, error_table, ("ws0", "ws1"))

        _fit_all_peaks_fitpeaks(
            wss=["ws0", "ws1"],
            peaks=[2.03],
            peak_window=0.02,
            save_dir="save",
            override_dir=True,
            i_over_sigma_thresh=3.0,
            nan_replacement="mean",
            no_fit_value_dict=None,
            peak_func_name="BackToBackExponential",
            max_fit_iters=50,
            fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
            smooth_vals=(3, 2),
        )

        # all workspaces combined into one ws for the peak, then converted to TOF
        self.mocks.clone.assert_called_once_with(InputWorkspace="crop_ws0", OutputWorkspace="__fitpeaks_combined_0")
        self.mocks.append.assert_called_once_with("__fitpeaks_combined_0", "crop_ws1", OutputWorkspace="__fitpeaks_combined_0")
        self.mocks.convert_units.assert_called_once()
        self.assertEqual(self.mocks.convert_units.call_args.kwargs["Target"], "TOF")

        # the per-spectrum TOF window is built once; the centre-seed workspace is rebuilt per FitPeaks
        # pass (2 smoothing passes + 1 raw fit here), so 1 window + 3 centres = 4 CreateWorkspace calls
        self.assertEqual(self.mocks.create_ws.call_count, 4)

        # rebunch-smoothing: one FitPeaks call on the raw data + one per smooth value (2 here)
        self.assertEqual(self.mocks.rebunch.call_count, 2)
        self.assertEqual(self.mocks.fitpeaks.call_count, 3)

        # smoothing passes run first (coarsest first), so the first call fits the rebunched workspace
        self.assertEqual(self.mocks.fitpeaks.call_args_list[0].kwargs["InputWorkspace"], "__fitpeaks_smooth_0_3")

        # the authoritative raw fit is the last call: it fits in TOF using the per-spectrum centre/window workspaces
        _, kw = self.mocks.fitpeaks.call_args_list[-1]
        self.assertEqual(kw["InputWorkspace"], combined_tof)
        self.assertEqual(kw["PeakCentersWorkspace"], "__fitpeaks_centres_0")
        self.assertEqual(kw["FitPeakWindowWorkspace"], "__fitpeaks_windows_0")
        self.assertEqual(kw["CostFunction"], "Unweighted least squares")
        self.assertEqual(kw["MinimumSignalToSigmaRatio"], 0)
        self.assertFalse(kw["HighBackground"])
        # no d-space shape seeds and no single-value centre/window list are passed
        self.assertNotIn("PeakParameterValues", kw)
        self.assertNotIn("PeakParameterNames", kw)
        self.assertNotIn("PeakCenters", kw)
        self.assertNotIn("FitWindowBoundaryList", kw)

        # result de-interleaved into one output table per workspace (2), each with 2 spectra
        self.assertEqual(self.mocks.populate.call_count, 2)
        self.assertEqual(self.mocks.save_nexus.call_count, 2)
        # ws1's slice (rows 2,3): raw fit fills the valid spectrum (I=7), spec 3 stays unfit (I=0)
        ws1_call = self.mocks.populate.call_args_list[1]
        np.testing.assert_array_equal(ws1_call[0][3]["I"], np.array([7.0, 0.0]))  # param_slices["I"] for ws1
        np.testing.assert_array_equal(ws1_call[0][6], np.array([True, False]))  # fit_mask slice for ws1

    def test_fit_all_peaks_fitpeaks_deletes_its_temporary_workspaces(self):
        # every ADS temporary the peak created (crops, combined, centres/windows, smoothed data, the
        # FitPeaks tables and its model/position output) is removed once the peak is done
        self.mocks.fit_summed.return_value = ([(0.95, 1.05)], [["crop_ws0"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        self._setup_combined_ws(2)
        param_table, error_table = _make_fit_tables([1.0, 1.0], [5.0, 6.0], [5000.0, 5000.0])
        self._setup_fit_tables(param_table, error_table, ("ws0",))

        _fit_all_peaks_fitpeaks(
            wss=["ws0"],
            peaks=[2.03],
            peak_window=0.02,
            save_dir="save",
            override_dir=True,
            i_over_sigma_thresh=3.0,
            nan_replacement="mean",
            no_fit_value_dict=None,
            peak_func_name="BackToBackExponential",
            max_fit_iters=50,
            fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
            smooth_vals=(3, 2),
        )

        deleted = {c[0][0] for c in self.mocks.delete_ws.call_args_list}
        self.assertLessEqual(
            {
                "crop_ws0",
                "rebin_ws_2.03",
                "peak_window_0",
                "__fitpeaks_combined_0",
                "__fitpeaks_centres_0",
                "__fitpeaks_windows_0",
                "__fitpeaks_smooth_0_3",
                "__fitpeaks_smooth_0_2",
                "__fitpeaks_params_0",
                "__fitpeaks_errs_0",
                "__fitpeaks_seed_0",
                "__fitpeaks_pos_0",
            },
            deleted,
        )
        # the reported output table is not a temporary
        self.assertNotIn("TEST123456_2.03_TestGroup_Fit_Parameters", deleted)

    def test_fit_all_peaks_fitpeaks_deletes_temporaries_when_a_peak_fails(self):
        # a failure part way through a peak must not leak the temporaries created before it
        self.mocks.fit_summed.return_value = ([(0.95, 1.05)], [["crop_ws0"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        self._setup_combined_ws(2)
        self.mocks.ads.retrieve.return_value = MagicMock()
        self.mocks.fitpeaks.side_effect = RuntimeError("fit blew up")

        with self.assertRaises(RuntimeError):
            _fit_all_peaks_fitpeaks(
                wss=["ws0"],
                peaks=[2.03],
                peak_window=0.02,
                save_dir="save",
                override_dir=True,
                i_over_sigma_thresh=3.0,
                nan_replacement="mean",
                no_fit_value_dict=None,
                peak_func_name="BackToBackExponential",
                max_fit_iters=50,
                fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
                smooth_vals=(3, 2),
            )

        deleted = {c[0][0] for c in self.mocks.delete_ws.call_args_list}
        self.assertLessEqual({"__fitpeaks_combined_0", "__fitpeaks_centres_0", "__fitpeaks_windows_0", "crop_ws0"}, deleted)

    def test_fit_all_peaks_fitpeaks_creates_the_output_directory(self):
        # without override_dir the results go into save_dir/grouping/peak, which SaveNexus will not create
        self.mocks.fit_summed.return_value = ([(0.95, 1.05)], [["crop_ws0"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        self._setup_combined_ws(2)
        param_table, error_table = _make_fit_tables([1.0, 1.0], [5.0, 6.0], [5000.0, 5000.0])
        self._setup_fit_tables(param_table, error_table, ("ws0",))

        _fit_all_peaks_fitpeaks(
            wss=["ws0"],
            peaks=[2.03],
            peak_window=0.02,
            save_dir="save",
            override_dir=False,
            i_over_sigma_thresh=3.0,
            nan_replacement="mean",
            no_fit_value_dict=None,
            peak_func_name="BackToBackExponential",
            max_fit_iters=50,
            fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
            smooth_vals=(3, 2),
        )

        made_dir = self.mocks.makedirs.call_args[0][0]
        self.assertEqual(made_dir, path.join("save", "TestGroup", "2.03"))
        self.assertTrue(self.mocks.makedirs.call_args.kwargs["exist_ok"])
        # the file itself still goes in that directory
        self.assertEqual(
            self.mocks.save_nexus.call_args.kwargs["Filename"],
            path.join(made_dir, "TEST123456_2.03_TestGroup_Fit_Parameters.nxs"),
        )

    def test_fit_all_peaks_fitpeaks_rejects_peaks_below_i_over_sigma_thresh(self):
        # one workspace, two spectra: both converge with a positive area (so the positive-area check
        # accepts both), but spec 1's intensity is not significant (I/I_err = 5/2 = 2.5) and must be
        # rejected by the i_over_sigma_thresh=3.0 mask, while spec 0 (I/I_err = 5/0.1 = 50) is kept.
        self.mocks.fit_summed.return_value = ([(0.95, 1.05)], [["crop_ws0"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        self._setup_combined_ws(2)

        param_table, error_table = _make_fit_tables([1.0, 1.0], [5.0, 5.0], [5000.0, 5000.0])
        error_table.column.side_effect = lambda p: [0.1, 2.0] if p == "I" else [0.1, 0.1]
        self._setup_fit_tables(param_table, error_table, ("ws0",))

        _fit_all_peaks_fitpeaks(
            wss=["ws0"],
            peaks=[2.03],
            peak_window=0.02,
            save_dir="save",
            override_dir=True,
            i_over_sigma_thresh=3.0,
            nan_replacement="mean",
            no_fit_value_dict=None,
            peak_func_name="BackToBackExponential",
            max_fit_iters=50,
            fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
            smooth_vals=(3, 2),
        )

        # the insignificant peak (spec 1) is masked out despite converging with a positive area
        ws0_call = self.mocks.populate.call_args_list[0]
        np.testing.assert_array_equal(ws0_call[0][6], np.array([True, False]))  # fit_mask slice for ws0

    def test_fit_all_peaks_fitpeaks_last_fit_ic_switches_final_function(self):
        # one workspace, two spectra; requested func is B2B but last_fit_ic switches the final fit to IC
        self.mocks.fit_summed.return_value = ([(0.95, 1.05)], [["crop_ws0"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        combined_tof = self._setup_combined_ws(2)
        param_table, error_table = _make_fit_tables([1.0, 1.0], [5.0, 6.0], [5000.0, 5000.0])
        self._setup_fit_tables(param_table, error_table, ("ws0",))

        _fit_all_peaks_fitpeaks(
            wss=["ws0"],
            peaks=[2.03],
            peak_window=0.02,
            save_dir="save",
            override_dir=True,
            i_over_sigma_thresh=3.0,
            nan_replacement="mean",
            no_fit_value_dict=None,
            peak_func_name="BackToBackExponential",
            max_fit_iters=50,
            fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
            smooth_vals=(3, 2),
            last_fit_ic=True,
        )

        # 2 smoothing passes + 1 raw B2B centre-refinement pass + 1 authoritative IC fit
        self.assertEqual(self.mocks.rebunch.call_count, 2)
        self.assertEqual(self.mocks.fitpeaks.call_count, 4)

        peak_funcs = [c.kwargs["PeakFunction"] for c in self.mocks.fitpeaks.call_args_list]
        self.assertEqual(
            peak_funcs,
            ["BackToBackExponential", "BackToBackExponential", "BackToBackExponential", "IkedaCarpenterPV"],
        )
        # the extra raw refinement (3rd call) and the authoritative IC fit (4th) both run on the raw
        # combined TOF workspace, not a rebunched one
        self.assertEqual(self.mocks.fitpeaks.call_args_list[2].kwargs["InputWorkspace"], combined_tof)
        self.assertEqual(self.mocks.fitpeaks.call_args_list[3].kwargs["InputWorkspace"], combined_tof)

        self.assertEqual(_n_output_tables(self.mocks.create_tab), 1)
        self.assertEqual(self.mocks.save_nexus.call_count, 1)

    def test_fit_all_peaks_fitpeaks_raises_on_non_uniform_spectra(self):
        # 2 workspaces but a combined ws with 3 spectra cannot be split evenly -> de-interleave fails
        self.mocks.fit_summed.return_value = ([(0.95, 1.05)], [["crop_ws0", "crop_ws1"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        self._setup_combined_ws(3)  # 3 % 2 != 0
        self.mocks.ads.retrieve.return_value = MagicMock()

        with self.assertRaises(RuntimeError) as ctx:
            _fit_all_peaks_fitpeaks(
                wss=["ws0", "ws1"],
                peaks=[2.03],
                peak_window=0.02,
                save_dir="save",
                override_dir=True,
                i_over_sigma_thresh=3.0,
                nan_replacement="mean",
                no_fit_value_dict=None,
                peak_func_name="BackToBackExponential",
                max_fit_iters=50,
                fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
                smooth_vals=(3, 2),
            )

        self.assertIn("cannot de-interleave", str(ctx.exception))
        # the guard fires before any fitting or output is done
        self.mocks.fitpeaks.assert_not_called()
        self.mocks.save_nexus.assert_not_called()

    def test_fit_all_peaks_fitpeaks_skips_smoothing_and_conversion_without_centre_param(self):
        # a peak function without an X0 centre param: no centre seed to refine, so the smoothing passes
        # are skipped and only the single authoritative fit runs; X0 has no TOF->d conversion either
        self.mocks.fit_summed.return_value = ([(0.95, 1.05)], [["crop_ws0"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "S"])
        self._setup_combined_ws(2)
        param_table, error_table = _make_fit_tables([1.0, 1.0], [5.0, 6.0], [0.0, 0.0])
        self._setup_fit_tables(param_table, error_table, ("ws0",))

        _fit_all_peaks_fitpeaks(
            wss=["ws0"],
            peaks=[2.03],
            peak_window=0.02,
            save_dir="save",
            override_dir=True,
            i_over_sigma_thresh=3.0,
            nan_replacement="mean",
            no_fit_value_dict=None,
            peak_func_name="SomeCentrelessFunc",
            max_fit_iters=50,
            fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
            smooth_vals=(3, 2),
        )

        # no centre param -> smoothing loop never runs, single authoritative fit only
        self.mocks.rebunch.assert_not_called()
        self.assertEqual(self.mocks.fitpeaks.call_count, 1)
        # X0 absent -> no per-spectrum TOF->d conversion of the centre
        self.mocks.toferr.assert_not_called()
        # results are still written
        self.assertEqual(self.mocks.save_nexus.call_count, 1)

    def test_fit_all_peaks_fitpeaks_iterates_over_multiple_peaks(self):
        # one workspace, two peaks: the peak loop runs once per peak, each producing its own output table
        self.mocks.fit_summed.return_value = ([(0.95, 1.05), (1.95, 2.05)], [["crop_p0"], ["crop_p1"]])
        self.mocks.func_factory.Instance.return_value.createPeakFunction.return_value = _make_peak_func_mock(["I", "A", "B", "X0", "S"])
        self._setup_combined_ws(2)
        param_table, error_table = _make_fit_tables([1.0, 1.0], [5.0, 6.0], [5000.0, 5000.0])
        self._setup_fit_tables(param_table, error_table, ("ws0",))

        _fit_all_peaks_fitpeaks(
            wss=["ws0"],
            peaks=[2.03, 2.5],
            peak_window=0.02,
            save_dir="save",
            override_dir=True,
            i_over_sigma_thresh=3.0,
            nan_replacement="mean",
            no_fit_value_dict=None,
            peak_func_name="BackToBackExponential",
            max_fit_iters=50,
            fit_kwargs={"Minimizer": "Levenberg-Marquardt"},
            smooth_vals=(3, 2),
        )

        # each peak combines its own crops (one ConvertUnits->TOF per peak) and writes one output table
        self.assertEqual(self.mocks.convert_units.call_count, 2)
        self.assertEqual(_n_output_tables(self.mocks.create_tab), 2)
        self.assertEqual(self.mocks.save_nexus.call_count, 2)
        self.assertEqual(self.mocks.populate.call_count, 2)
        # per peak: 2 smoothing passes + 1 raw fit, and 2 rebunches
        self.assertEqual(self.mocks.fitpeaks.call_count, 6)
        self.assertEqual(self.mocks.rebunch.call_count, 4)


class FitPeaksSeedCarryTests(unittest.TestCase):
    """The per-pass centre/shape carry-forward helpers, driven directly."""

    @patch(f"{fitpeaks_path}.ADS")
    def test_refine_centre_seed_only_accepts_centres_inside_the_fit_window(self, mock_ads):
        # spectrum 0 refines (fitted centre inside its window); spectrum 1 was fitted outside its
        # window, 2 did not converge and 3 returned a non-finite centre - all three keep their seed
        tof_lo, tof_hi = np.full(4, 100.0), np.full(4, 200.0)
        param_table = MagicMock()
        param_table.column.return_value = [150.0, 500.0, 150.0, np.nan]
        mock_ads.retrieve.return_value = param_table
        seed = np.array([120.0, 120.0, 120.0, 120.0])

        refined = _refine_centre_seed("__params", seed, np.array([True, True, False, True]), tof_lo, tof_hi)

        np.testing.assert_array_equal(refined, np.array([150.0, 120.0, 120.0, 120.0]))
        param_table.column.assert_called_once_with("X0")

    @patch(f"{fitpeaks_path}.CreateEmptyTableWorkspace")
    @patch(f"{fitpeaks_path}.ADS")
    def test_build_seed_table_writes_nan_for_failed_spectra(self, mock_ads, mock_create_tab):
        # a failed spectrum (or a non-finite fitted value) must be NaN so FitPeaks falls back to the
        # instrument parameters for it rather than being poisoned by a bad guiding fit
        src = MagicMock()
        src.column.side_effect = lambda p: {"A": [1.0, 2.0, np.inf], "B": [3.0, 4.0, 5.0]}[p]
        mock_ads.retrieve.return_value = src
        tab = MagicMock()
        mock_create_tab.return_value = tab

        name = _build_seed_table("__params", "__seed", ["A", "B"], np.array([True, False, True]), 3)

        self.assertEqual(name, "__seed")
        rows = [c[0][0] for c in tab.addRow.call_args_list]
        np.testing.assert_array_equal(rows[0], [1.0, 3.0])  # valid fit carried forward
        self.assertTrue(all(np.isnan(v) for v in rows[1]))  # failed spectrum
        self.assertTrue(np.isnan(rows[2][0]))  # non-finite value dropped
        self.assertEqual(rows[2][1], 5.0)  # its other parameter still carried

    @patch(f"{fitpeaks_path}.CreateEmptyTableWorkspace")
    @patch(f"{fitpeaks_path}.ADS")
    def test_build_seed_table_returns_none_without_carried_parameters(self, mock_ads, mock_create_tab):
        # nothing to carry -> no table at all, so the next pass seeds from the instrument parameters
        self.assertIsNone(_build_seed_table("__params", "__seed", [], np.array([True]), 1))
        mock_create_tab.assert_not_called()
        mock_ads.retrieve.assert_not_called()


if __name__ == "__main__":
    unittest.main()
