# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch, create_autospec, MagicMock
import numpy as np
from numpy import allclose, zeros_like, ones, trapezoid, array, linspace
from numpy.testing import assert_array_equal, assert_array_almost_equal
from mantid.api import AnalysisDataService, FileFinder
from mantid.simpleapi import CreateWorkspace, FlatBackground, CloneWorkspace, MaskDetectors, ExtractSpectra
from Engineering.pawley_utils import Phase, GaussianProfile, PawleyPattern1D
from Engineering.poldi_pawley_utils import PawleyPattern2D
from plugins.algorithms.poldi_utils import (
    load_poldi,
    _do_interp_with_flux_correction,
    _get_flux_arrays,
    simulate_2d_data,
    get_dspac_array_from_ws,
    get_dspac_limits_from_ws,
)


class PawleyPattern2DTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")  # for np.loadtxt so need full path
        cls.ws = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")  # can be changed by the class
        # need all HKL in range otherwise polyfit doesn't work when global_scale=False
        self.phase.set_hkls_from_dspac_limits(0.7, 3.5)

    def test_set_global_scale_false_no_bg(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        pawley.set_global_scale(False)
        self.assertEqual(len(pawley.bg_params), 0)

    def test_set_global_scale_false_with_bg(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile(), bg_func=FlatBackground(A0=2))
        pawley.set_global_scale(False)
        self.asser_allclose(pawley.bg_params, 0)  # zero global bg (optimised with scale for each spectrum)
        self.assertFalse(pawley.bg_isfree.any(), 0)  # background fixed

    def test_eval_profile(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        ycalc = pawley.eval_profile(pawley.get_free_params())
        # assert total intensity = 1 (default peak intensity is 1, there is only 1 peak and no background)
        self.assertAlmostEqual(trapezoid(ycalc, pawley.ws_1d.x(0)), self.phase.nhkls(), delta=1e-1)

    def test_eval_resids_global_scale_true(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        resids = pawley.eval_resids(pawley.get_free_params())
        self.assertLess(sum(resids), 0)  # check global scale gives an over estimate i.e. resid is negative

    def test_eval2D_respects_lambda_max(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, lambda_max=5.0, profile=GaussianProfile())
        ysum = pawley.eval_2d(pawley.get_free_params()).extractY().sum()
        pawley.lambda_max = 2.0
        ysum_cropped = pawley.eval_2d(pawley.get_free_params()).extractY().sum()
        self.assertLess(ysum_cropped, ysum)

    def test_eval_resids_global_scale_false(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=False, profile=GaussianProfile())

        pawley._reestimate_scales(pawley.get_free_params())
        resids = pawley.eval_resids(pawley.get_free_params())

        # sum of resids should be much smaller than global-scale=True (as background optimised)
        self.assertAlmostEqual(sum(resids), 0, delta=4e-10)

    @patch("Engineering.poldi_pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.poldi_pawley_utils.logger")
    def test_set_params_from_pawley1d_different_number_phases(self, mock_log, mock_estimate_intens):
        mock_pawley1d = self._make_mock_pawley1d()
        mock_pawley1d.phases = 2 * mock_pawley1d.phases
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=False, profile=GaussianProfile())
        initial_profile = pawley.profile_params[0].copy()
        intial_intens = pawley.intens[0].copy()
        pawley.set_params_from_pawley1d(mock_pawley1d)

        mock_log.error.assert_called_once()
        assert_array_almost_equal(pawley.intens[0], intial_intens)
        assert_array_almost_equal(pawley.profile_params[0], initial_profile)
        mock_estimate_intens.assert_not_called()

    @patch("Engineering.poldi_pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.poldi_pawley_utils.logger")
    def test_set_params_from_pawley1d_different_nhkls(self, mock_log, mock_estimate_intens):
        mock_pawley1d = self._make_mock_pawley1d()
        mock_pawley1d.phases[0].get_param_names.return_value = self.phase.get_param_names()
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=False, profile=GaussianProfile())
        intial_intens = pawley.intens[0].copy()

        pawley.set_params_from_pawley1d(mock_pawley1d)

        mock_log.error.assert_not_called()
        assert_array_almost_equal(pawley.intens[0], intial_intens)
        assert_array_almost_equal(pawley.profile_params[0], mock_pawley1d.profile_params[0])
        mock_estimate_intens.assert_not_called()

    @patch("Engineering.poldi_pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.poldi_pawley_utils.logger")
    def test_set_params_from_pawley1d_successful(self, mock_log, mock_estimate_intens):
        mock_pawley1d = self._make_mock_pawley1d()
        mock_pawley1d.phases[0].get_param_names.return_value = self.phase.get_param_names()
        mock_pawley1d.phases[0].nhkls.return_value = self.phase.nhkls()
        mock_pawley1d.intens[0] = 2 * ones(self.phase.nhkls())  # non-default value
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=False, profile=GaussianProfile())

        pawley.set_params_from_pawley1d(mock_pawley1d)

        mock_log.error.assert_not_called()
        assert_array_almost_equal(pawley.intens[0], mock_pawley1d.intens[0])
        assert_array_almost_equal(pawley.profile_params[0], mock_pawley1d.profile_params[0])
        mock_estimate_intens.assert_called_once()

    def _make_mock_pawley1d(self):
        mock_pawley1d = create_autospec(PawleyPattern1D)
        mock_pawley1d.profile = GaussianProfile()
        mock_pawley1d.profile_params = [ones(3)]  # non-default values
        mock_pawley1d.phases = [create_autospec(Phase)]
        mock_pawley1d.phases[0].nhkls.return_value = 1
        mock_pawley1d.intens = [ones(1)]
        mock_pawley1d.alatt_params = [ones(3)]
        return mock_pawley1d

    # --- param_bounds tests ---

    def test_param_bounds_default_values(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], profile=GaussianProfile())
        self.assertEqual(pawley._param_bounds, {})
        self.assertAlmostEqual(pawley.param_bounds_abs_min, 1e-6)

    def test_set_bounds_all_fractional(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], profile=GaussianProfile(), param_bounds_abs_min=1e-4)
        pawley.set_bounds_all(mode="fractional", value=0.1)
        # every parameter should now have stored (lb, ub) bounds
        for name in pawley.get_param_names():
            self.assertIn(name, pawley._param_bounds)

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.pawley_utils.least_squares")
    def test_fit_passes_bounds_computed_from_initial_params(self, mock_ls, mock_err):
        frac, abs_min = 0.1, 1e-4
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile(), param_bounds_abs_min=abs_min)
        pawley.set_bounds_all(mode="fractional", value=frac)
        initial_params = pawley.get_free_params()
        mock_ls.return_value = MagicMock(x=initial_params, cost=10)
        mock_err.return_value = np.zeros_like(pawley.get_free_params())
        pawley.fit()
        lb, ub = mock_ls.call_args[1]["bounds"]
        expected_margin = np.maximum(np.abs(initial_params) * frac, abs_min)
        assert_array_almost_equal(lb, initial_params - expected_margin)
        assert_array_almost_equal(ub, initial_params + expected_margin)

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.pawley_utils.least_squares")
    def test_fit_uses_abs_min_floor_for_zero_valued_params(self, mock_ls, mock_err):
        abs_min = 1e-4
        # FlatBackground(A0=0) with global_scale=True gives a zero-valued free param
        pawley = PawleyPattern2D(
            self.ws,
            [self.phase],
            global_scale=True,
            profile=GaussianProfile(),
            bg_func=FlatBackground(A0=0),
            param_bounds_abs_min=abs_min,
        )
        pawley.set_bounds_all(mode="fractional", value=0.1)
        initial_params = pawley.get_free_params()
        mock_ls.return_value = MagicMock(x=initial_params, cost=10)
        mock_err.return_value = np.zeros_like(pawley.get_free_params())
        pawley.fit()
        lb, ub = mock_ls.call_args[1]["bounds"]
        zero_mask = initial_params == 0
        self.assertTrue(zero_mask.any())
        assert_array_almost_equal(ub[zero_mask], np.full(zero_mask.sum(), abs_min))
        assert_array_almost_equal(lb[zero_mask], np.full(zero_mask.sum(), -abs_min))

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.pawley_utils.least_squares")
    def test_fit_does_not_override_explicit_bounds(self, mock_ls, mock_err):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        pawley.set_bounds_all(mode="fractional", value=0.1)
        n = len(pawley.get_free_params())
        mock_ls.return_value = MagicMock(x=pawley.get_free_params(), cost=10)
        mock_err.return_value = np.zeros_like(pawley.get_free_params())
        explicit_bounds = (-np.inf * np.ones(n), np.inf * np.ones(n))
        pawley.fit(bounds=explicit_bounds)
        self.assertIs(mock_ls.call_args[1]["bounds"], explicit_bounds)

    @patch("Engineering.poldi_pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.poldi_pawley_utils.logger")
    def test_set_params_from_pawley1d_preserves_existing_bounds(self, mock_log, mock_estimate_intens):
        mock_pawley1d = self._make_mock_pawley1d()
        mock_pawley1d.phases[0].nhkls.return_value = self.phase.nhkls()
        mock_pawley1d.intens[0] = 2 * ones(self.phase.nhkls())
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=False, profile=GaussianProfile(), param_bounds_abs_min=1e-3)
        pawley.set_bounds_all(mode="fractional", value=0.3)
        pawley.set_params_from_pawley1d(mock_pawley1d)
        self.assertAlmostEqual(pawley.param_bounds_abs_min, 1e-3)
        # stored bounds should be unchanged after set_params_from_pawley1d
        self.assertTrue(len(pawley._param_bounds) > 0)

    def test_create_no_constraints_fit_forwards_abs_min(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        no_constr = pawley.create_no_constraints_fit(param_bounds_abs_min=1e-5)
        self.assertAlmostEqual(no_constr.param_bounds_abs_min, 1e-5)

    def test_create_no_constraints_fit_default_no_bounds(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        no_constr = pawley.create_no_constraints_fit()
        self.assertEqual(no_constr._param_bounds, {})
        self.assertAlmostEqual(no_constr.param_bounds_abs_min, 1e-6)


class PawleyPattern2DNoConstraintsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")  # for np.loadtxt so need full path
        cls.ws = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")  # can be changed by the class
        self.phase.set_hkls_from_dspac_limits(1.9, 3.5)  # 2 peaks
        self.init_kwargs = {"ws": self.ws, "phases": [self.phase], "profile": GaussianProfile()}

    def test_global_scale_false_no_bg(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=False).create_no_constraints_fit()
        # note intensity on first peak fixed
        assert_array_equal(pawley.get_isfree(), array([True, True, True, True, True, True]))

    def test_global_scale_true_no_bg(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True).create_no_constraints_fit()
        # no intensities fixed
        assert_array_equal(pawley.get_isfree(), ones(6, dtype=bool))

    def test_global_scale_true_with_bg(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True, bg_func=FlatBackground(A0=2)).create_no_constraints_fit()
        # no intensities or bg fixed
        assert_array_equal(pawley.get_isfree(), ones(7, dtype=bool))

    def test_global_scale_false_with_bg(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=False, bg_func=FlatBackground(A0=2)).create_no_constraints_fit()
        assert_array_equal(pawley.get_isfree(), array([True, True, True, True, True, True, False]))

    def test_fit(self):
        # run fit with 2 func eval to check no error, result returned and params changed
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True).create_no_constraints_fit()
        result = pawley.fit(max_nfev=2)

        self.assertEqual(result.nfev, 2)
        # assert parameters changed
        self.assertFalse(allclose(pawley.initial_params, pawley.get_free_params()))

    # --- param_bounds tests ---

    def test_param_bounds_default_values(self):
        pawley = PawleyPattern2D(**self.init_kwargs).create_no_constraints_fit()
        self.assertEqual(pawley._param_bounds, {})
        self.assertAlmostEqual(pawley.param_bounds_abs_min, 1e-6)

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.poldi_pawley_utils.least_squares")
    def test_fit_passes_bounds_computed_from_initial_params(self, mock_ls, mock_err):
        frac, abs_min = 0.1, 1e-4
        pawley = PawleyPattern2D(**self.init_kwargs).create_no_constraints_fit(param_bounds_abs_min=abs_min)
        pawley.set_bounds_all(mode="fractional", value=frac)
        initial_params = pawley.get_free_params()
        mock_ls.return_value = MagicMock(x=initial_params)
        mock_err.return_value = np.zeros_like(pawley.get_free_params())
        pawley.fit()
        lb, ub = mock_ls.call_args[1]["bounds"]
        expected_margin = np.maximum(np.abs(initial_params) * frac, abs_min)
        assert_array_almost_equal(lb, initial_params - expected_margin)
        assert_array_almost_equal(ub, initial_params + expected_margin)

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.poldi_pawley_utils.least_squares")
    def test_fit_uses_abs_min_floor_for_zero_valued_params(self, mock_ls, mock_err):
        abs_min = 1e-4
        # FlatBackground(A0=0) with global_scale=True gives a zero-valued free param
        init_kwargs_with_bg = {**self.init_kwargs, "global_scale": True, "bg_func": FlatBackground(A0=0)}
        pawley = PawleyPattern2D(**init_kwargs_with_bg).create_no_constraints_fit(param_bounds_abs_min=abs_min)
        pawley.set_bounds_all(mode="fractional", value=0.1)
        initial_params = pawley.get_free_params()
        mock_ls.return_value = MagicMock(x=initial_params)
        mock_err.return_value = np.zeros_like(pawley.get_free_params())
        pawley.fit()
        lb, ub = mock_ls.call_args[1]["bounds"]
        zero_mask = initial_params == 0
        self.assertTrue(zero_mask.any())
        assert_array_almost_equal(ub[zero_mask], np.full(zero_mask.sum(), abs_min))
        assert_array_almost_equal(lb[zero_mask], np.full(zero_mask.sum(), -abs_min))

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.poldi_pawley_utils.least_squares")
    def test_fit_does_not_override_explicit_bounds(self, mock_ls, mock_err):
        pawley = PawleyPattern2D(**self.init_kwargs).create_no_constraints_fit()
        pawley.set_bounds_all(mode="fractional", value=0.1)
        n = len(pawley.get_free_params())
        mock_ls.return_value = MagicMock(x=pawley.get_free_params())
        mock_err.return_value = np.zeros_like(pawley.get_free_params())
        explicit_bounds = (-np.inf * np.ones(n), np.inf * np.ones(n))
        pawley.fit(bounds=explicit_bounds)
        self.assertIs(mock_ls.call_args[1]["bounds"], explicit_bounds)

    # --- create_output_tables tests ---

    def test_create_output_tables_single_phase_returns_one_table(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True).create_no_constraints_fit()
        pawley.fit(max_nfev=1)
        tables = pawley.create_output_tables()
        self.assertEqual(len(tables), 1)

    def test_create_output_tables_single_phase_hkl_content(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True).create_no_constraints_fit()
        pawley.fit(max_nfev=1)
        pawley.phases[0].set_phase_name("Si")
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].column("HKL"), self.phase.get_hkl_strings())

    def test_create_output_tables_two_phases_correct_hkl_split(self):
        # phase covers 1.9-3.5 Å (2 peaks); phase2 covers 3.5-4.5 Å (different peaks)
        phase2 = Phase.from_alatt(3 * [5.43094], "F d -3 m")
        phase2.set_hkls_from_dspac_limits(3.5, 4.5)
        pawley2d = PawleyPattern2D(self.ws, [self.phase, phase2], global_scale=True, profile=GaussianProfile())
        pawley = pawley2d.create_no_constraints_fit()
        pawley.fit(max_nfev=1)
        for iphase, phase_name in enumerate(["low_d", "high_d"]):
            pawley.phases[iphase].set_phase_name(phase_name)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].column("HKL"), self.phase.get_hkl_strings())
        self.assertEqual(tables[1].column("HKL"), phase2.get_hkl_strings())


class PawleyPattern2DNoConstraintsPopulateTableTest(unittest.TestCase):
    """Test PawleyPattern2DNoConstraints._populate_table with phases=None."""

    @classmethod
    def setUpClass(cls):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        cls.ws = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def test_populate_table_phases_none(self):
        phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")
        phase.set_hkls_from_dspac_limits(1.9, 3.5)
        pawley2d = PawleyPattern2D(self.ws, [phase], global_scale=True, profile=GaussianProfile())
        no_constr = pawley2d.create_no_constraints_fit()
        no_constr.fit(max_nfev=1)
        # set phases to None and verify _populate_table still works using peak index as HKL string
        no_constr.phases = None
        tables = no_constr.create_output_tables()
        self.assertEqual(len(tables), 1)
        # HKL column should contain string indices ("0", "1", ...) when phases is None
        hkl_col = tables[0].column("HKL")
        for i, hkl_str in enumerate(hkl_col):
            self.assertEqual(hkl_str, str(i))


class PawleyPattern2DFitAlternatingTest(unittest.TestCase):
    """Tests for PawleyPattern2D.fit alternating loop."""

    @classmethod
    def setUpClass(cls):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        cls.ws = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")
        self.phase.set_hkls_from_dspac_limits(1.9, 3.5)

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.pawley_utils.least_squares")
    def test_max_scale_iter_respected(self, mock_ls, mock_errs):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        # make each iteration return different cost so no early termination
        results = []
        for i in range(5):
            r = MagicMock()
            r.x = pawley.get_free_params()
            r.cost = 100.0 - i * 10  # decreasing cost, never converges
            results.append(r)
        mock_ls.side_effect = results
        mock_errs.return_value = np.zeros_like(pawley.get_free_params())

        pawley.fit_with_strategy(strategy="alternating", max_scale_iter=3, max_nfev=1)
        self.assertEqual(mock_ls.call_count, 3)

    @patch("Engineering.pawley_utils.OutputTableMixin.get_parameter_errors")
    @patch("Engineering.pawley_utils.least_squares")
    def test_scales_reset_between_iterations(self, mock_ls, mock_errs):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        # track scales being set to None at each iteration
        scales_before_eval = []
        original_eval_2d = pawley.eval_2d

        def tracking_eval_2d(params):
            scales_before_eval.append(pawley.scales)
            return original_eval_2d(params)

        pawley.eval_2d = tracking_eval_2d

        results = []
        for i in range(3):
            r = MagicMock()
            r.x = pawley.get_free_params()
            r.cost = 100.0 - i * 10
            results.append(r)
        mock_ls.side_effect = results
        mock_errs.return_value = np.zeros_like(pawley.get_free_params())

        pawley.fit_with_strategy(strategy="alternating", max_scale_iter=2, max_nfev=1)
        # scales should be reset to ones before each eval_2d call in the loop
        for scales_val in scales_before_eval[:2]:
            np.testing.assert_array_equal(scales_val, np.ones(self.ws.getNumberHistograms()))


class PoldiUtilsFluxTest(unittest.TestCase):
    """Tests for poldi_utils flux correction functions."""

    def test_do_interp_with_flux_correction(self):
        # simple 1D case: uniform flux should match plain interpolation scaled by flux value
        d = linspace(1.0, 5.0, 50)
        intensity = np.sin(d)
        sin_theta = 0.5
        lam_grid = linspace(0.5, 6.0, 100)
        flux_vals = ones(100)  # uniform flux = 1.0

        dtarget = linspace(1.5, 4.5, 30)[:, None]  # 2D for sum(axis=1)
        result = _do_interp_with_flux_correction(dtarget, d, intensity, sin_theta, lam_grid, flux_vals)
        # with uniform flux of 1.0, should be same as plain interp
        expected = np.interp(dtarget.flatten(), d, intensity)
        assert_array_almost_equal(result, expected, decimal=10)

    def test_do_interp_with_flux_correction_non_uniform_flux(self):
        d = linspace(1.0, 5.0, 50)
        intensity = ones(50)
        sin_theta = 0.5
        lam_grid = linspace(0.5, 6.0, 100)
        flux_vals = 2.0 * ones(100)  # uniform flux = 2.0

        dtarget = linspace(1.5, 4.5, 30)[:, None]
        result = _do_interp_with_flux_correction(dtarget, d, intensity, sin_theta, lam_grid, flux_vals)
        # intensity * flux = 1 * 2 = 2 everywhere, so interpolated result should be ~2
        assert_array_almost_equal(result, 2.0 * ones(30), decimal=5)

    def test_get_flux_arrays(self):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        ws = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)
        lam_grid, flux_vals = _get_flux_arrays(ws, lam_min=1.0, lam_max=5.0, n_points=50)
        self.assertEqual(len(lam_grid), 50)
        self.assertEqual(len(flux_vals), 50)
        self.assertAlmostEqual(lam_grid[0], 1.0)
        self.assertAlmostEqual(lam_grid[-1], 5.0)
        # flux values should be positive
        self.assertTrue(np.all(flux_vals >= 0))
        AnalysisDataService.clear()

    @patch("plugins.algorithms.poldi_utils._do_interp_with_flux_correction")
    @patch("plugins.algorithms.poldi_utils._do_interp")
    def test_flux_sample_points_none_disables_flux_correction(self, mock_interp, mock_flux_interp):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        ws_2d = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)
        # create a simple 1D workspace for input
        phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")
        phase.set_hkls([[1, 1, 1]])
        dspacs = linspace(0.69, 4.15, 2460)
        ws_1d = CreateWorkspace(DataX=dspacs, DataY=zeros_like(dspacs), UnitX="dSpacing", OutputWorkspace="ws_1d_flux_test")
        mock_interp.return_value = zeros_like(ws_2d.x(0))

        simulate_2d_data(ws_2d, ws_1d, flux_sample_points=None)

        mock_flux_interp.assert_not_called()
        self.assertGreater(mock_interp.call_count, 0)
        AnalysisDataService.clear()


class PoldiUtilsMaskingTest(unittest.TestCase):
    """Tests that the poldi_utils simulation and d-spacing helpers exclude masked spectra."""

    IMASK = 2  # interior spectrum, so masking it does not alter the two-theta/L2 range

    @classmethod
    def setUpClass(cls):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        cls.ws_full = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)
        # crop to a handful of spectra to keep the simulation quick
        cls.ws_2d = ExtractSpectra(InputWorkspace=cls.ws_full, StartWorkspaceIndex=0, EndWorkspaceIndex=4, OutputWorkspace="ws_2d_crop")
        # single sharp peak so the simulated spectra are non-trivial
        dspacs = linspace(0.69, 4.15, 2460)
        cls.ws_1d = CreateWorkspace(
            DataX=dspacs, DataY=np.exp(-0.5 * ((dspacs - 3.135) / 0.01) ** 2), UnitX="dSpacing", OutputWorkspace="ws_1d_mask_test"
        )

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def _make_masked_ws(self):
        ws_masked = CloneWorkspace(InputWorkspace=self.ws_2d, OutputWorkspace="ws_2d_masked")
        MaskDetectors(Workspace=ws_masked, WorkspaceIndexList=[self.IMASK])
        return ws_masked

    def test_simulate_2d_data_zeroes_masked_spectra(self):
        ws_sim_ref = simulate_2d_data(self.ws_2d, self.ws_1d, output_workspace="ws_sim_ref")
        ws_sim = simulate_2d_data(self._make_masked_ws(), self.ws_1d, output_workspace="ws_sim_masked")

        # the masked spectrum is zeroed, not simulated (it is non-zero when unmasked)
        self.assertTrue(np.any(ws_sim_ref.y(self.IMASK) > 0))
        assert_array_equal(ws_sim.y(self.IMASK), zeros_like(ws_sim.y(self.IMASK)))
        # the remaining spectra are unaffected
        for ispec in (0, 1, 3, 4):
            assert_array_almost_equal(ws_sim.y(ispec), ws_sim_ref.y(ispec))

    def test_simulate_2d_data_zeroes_masked_spectra_holding_counts(self):
        # masking does not always zero the counts, so the simulation must zero them itself
        # rather than leaving the values carried over from the input workspace
        ws_masked = self._make_masked_ws()
        ws_masked.setSharedY(self.IMASK, ones(ws_masked.blocksize()))

        ws_sim = simulate_2d_data(ws_masked, self.ws_1d, output_workspace="ws_sim_masked")

        assert_array_equal(ws_sim.y(self.IMASK), zeros_like(ws_sim.y(self.IMASK)))

    def _make_low_tth_masked_ws(self):
        # the first spectra have the lowest two-theta, so masking them lowers the maximum d-spacing
        ws_masked = CloneWorkspace(InputWorkspace=self.ws_full, OutputWorkspace="ws_full_masked")
        MaskDetectors(Workspace=ws_masked, WorkspaceIndexList=list(range(20)))
        return ws_masked

    def test_get_dspac_limits_from_ws_excludes_masked_spectra(self):
        _, dmax_ref = get_dspac_limits_from_ws(self.ws_full)
        _, dmax = get_dspac_limits_from_ws(self._make_low_tth_masked_ws())

        self.assertLess(dmax, dmax_ref)

    def test_get_dspac_array_from_ws_excludes_masked_spectra(self):
        dspacs_ref = get_dspac_array_from_ws(self.ws_full)
        dspacs = get_dspac_array_from_ws(self._make_low_tth_masked_ws())

        self.assertLess(dspacs[-1], dspacs_ref[-1])
        self.assertAlmostEqual(dspacs[0], dspacs_ref[0])  # min d-spacing set by the highest two-theta


class Poldi2DEvalMixinMaskingTest(unittest.TestCase):
    """Tests that masked spectra are excluded from the 2D Pawley residuals and scale estimation."""

    IMASK = 2  # interior spectrum, so masking it does not alter the two-theta/L2 range

    @classmethod
    def setUpClass(cls):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        ws = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)
        # crop to a handful of spectra to keep the simulation quick
        cls.ws = ExtractSpectra(InputWorkspace=ws, StartWorkspaceIndex=0, EndWorkspaceIndex=4, OutputWorkspace="ws_2d_eval_crop")

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")
        # need all HKL in range otherwise polyfit doesn't work when global_scale=False
        self.phase.set_hkls_from_dspac_limits(0.7, 3.5)

    def _make_masked_pawley(self, global_scale=True):
        ws_masked = CloneWorkspace(InputWorkspace=self.ws, OutputWorkspace="ws_2d_eval_masked")
        MaskDetectors(Workspace=ws_masked, WorkspaceIndexList=[self.IMASK])
        return PawleyPattern2D(ws_masked, [self.phase], global_scale=global_scale, profile=GaussianProfile())

    def _ilive(self):
        return [ispec for ispec in range(self.ws.getNumberHistograms()) if ispec != self.IMASK]

    def test_eval_resids_excludes_masked_spectra(self):
        pawley = self._make_masked_pawley()

        nresids = np.asarray(pawley.eval_resids(pawley.get_free_params())).size

        self.assertEqual(nresids, (self.ws.getNumberHistograms() - 1) * self.ws.blocksize())

    def test_eval_resids_uses_mask_applied_after_construction(self):
        ws = CloneWorkspace(InputWorkspace=self.ws, OutputWorkspace="ws_2d_eval_masked_later")
        pawley = PawleyPattern2D(ws, [self.phase], profile=GaussianProfile())
        nresids = np.asarray(pawley.eval_resids(pawley.get_free_params())).size

        MaskDetectors(Workspace=ws, WorkspaceIndexList=[self.IMASK])
        nresids_masked = np.asarray(pawley.eval_resids(pawley.get_free_params())).size

        self.assertEqual(nresids - nresids_masked, self.ws.blocksize())

    def test_eval_resids_ignores_counts_in_masked_spectra(self):
        pawley = self._make_masked_pawley()
        resids = np.asarray(pawley.eval_resids(pawley.get_free_params()))

        # masking does not always zero the counts, so put some back and check they are ignored
        pawley.ws.setSharedY(self.IMASK, 1e4 * ones(pawley.ws.blocksize()))
        resids_with_counts = np.asarray(pawley.eval_resids(pawley.get_free_params()))

        assert_array_almost_equal(resids, resids_with_counts)

    def test_reestimate_scales_leaves_masked_spectra_neutral(self):
        pawley = self._make_masked_pawley(global_scale=False)

        scales, bgs = pawley._reestimate_scales(pawley.get_free_params())

        self.assertEqual(scales[self.IMASK], 1.0)
        self.assertEqual(bgs[self.IMASK], 0.0)
        # the unmasked spectra are estimated rather than left at the neutral defaults
        self.assertFalse(np.any(scales[self._ilive()] == 1.0))

    def test_estimate_intensities_excludes_masked_spectra_from_median_scale(self):
        pawley = self._make_masked_pawley(global_scale=False)
        intens_before = pawley.intens[0].copy()
        # _estimate_intensities re-estimates the scales itself, so this gives the same values
        scales, _ = pawley._reestimate_scales(pawley.get_free_params())
        expected_scale = np.median(scales[self._ilive()])

        pawley._estimate_intensities()

        # the neutral scale of 1.0 on the masked spectrum must not pull the median
        self.assertNotAlmostEqual(expected_scale, np.median(scales))
        assert_array_almost_equal(pawley.intens[0], intens_before * expected_scale)


if __name__ == "__main__":
    unittest.main()
