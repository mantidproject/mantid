# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch, create_autospec
from numpy import allclose, log, zeros_like, ones, trapezoid, array, linspace, sqrt
from numpy.testing import assert_array_equal, assert_array_almost_equal
from mantid.api import AnalysisDataService, FileFinder
from mantid.simpleapi import CreateWorkspace, FlatBackground, EditInstrumentGeometry, ConvertUnits, LinearBackground
from mantid.geometry import CrystalStructure
from Engineering.pawley_utils import Phase, GaussianProfile, PVProfile, PawleyPattern1D, PawleyPattern2D, BackToBackGauss
from plugins.algorithms.poldi_utils import load_poldi


class PhaseTest(unittest.TestCase):
    SI_LATT_PAR = 5.43094
    SI_SPGR = "F d -3 m"

    def test_init(self):
        si = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        self._assert_si_phase(si)

    def test_init_phase_from_cif(self):
        si = Phase.from_cif("SI.cif")
        self._assert_si_phase(si)

    def test_init_phase_from_alatt(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        self._assert_si_phase(si)

    def test_set_hkls_from_dspac_limits(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        si.set_hkls_from_dspac_limits(2, 3.5)  # (1,1,1) peak only
        self.assertTrue(allclose(si.hkls, 1))
        self.assertEqual(si.nhkls(), 1)

    def test_set_hkls(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        si.set_hkls([[1, 1, 1]])
        self.assertTrue(allclose(si.hkls, 1))
        self.assertEqual(si.nhkls(), 1)

    @patch("Engineering.pawley_utils.logger")
    def test_set_hkls_forbidden(self, mock_log):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        si.set_hkls([[1, 1, 1], [1, 1, 0]])
        self.assertTrue(allclose(si.hkls, 1))
        self.assertEqual(si.nhkls(), 1)
        mock_log.warning.assert_called_once()

    def test_get_params_noncubic(self):
        tetrag_phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], "P 4")
        assert_array_almost_equal(tetrag_phase.get_params(), array(2 * [self.SI_LATT_PAR]))

    def _assert_si_phase(self, phase):
        pars = phase.get_params()
        self.assertEqual(len(pars), 1)  # only 1 lattice parameter in cubic system (a)
        self.assertAlmostEqual(pars[0], self.SI_LATT_PAR, delta=1e-5)
        self.assertEqual(phase.spgr.getHMSymbol(), self.SI_SPGR)
        self.assertEqual(phase.get_param_names()[0], "a")
        self.assertEqual(
            str(phase.unit_cell), "UnitCell with lattice parameters: a = 5.43094 b = 5.43094 c = 5.43094 alpha = 90 beta = 90 gamma = 90"
        )


class ProfileTest(unittest.TestCase):
    DSPAC = 1.5
    GAUSS_SIGMA = 0.00158  # at d=1.5 Ang

    def test_gaussian_profile(self):
        profile = GaussianProfile()
        self.assertAlmostEqual(profile.get_mantid_peak_params(self.DSPAC)["Sigma"], self.GAUSS_SIGMA, delta=1e-5)
        self.assertEqual(profile.func_name, "Gaussian")

    def test_pvoigt_profile(self):
        profile = PVProfile()
        param_dict = profile.get_mantid_peak_params(self.DSPAC)
        self.assertAlmostEqual(param_dict["FWHM"], 2 * sqrt(2 * log(2)) * self.GAUSS_SIGMA, delta=1e-5)
        self.assertAlmostEqual(param_dict["Mixing"], 0, delta=1e-8)
        self.assertEqual(profile.func_name, "PseudoVoigt")

    def test_back_to_back_gauss(self):
        profile = BackToBackGauss()
        param_dict = profile.get_mantid_peak_params(self.DSPAC)
        self.assertAlmostEqual(param_dict["A"], 0.0645, delta=1e-4)
        self.assertAlmostEqual(param_dict["B"], 0.0240, delta=1e-4)
        self.assertAlmostEqual(param_dict["S"], 20.0, delta=1e-1)
        self.assertEqual(profile.func_name, "BackToBackExponential")


class PawleyPattern1DTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        dspacs = linspace(0.69, 4.15, 2460)
        cls.ws = CreateWorkspace(
            DataX=dspacs, DataY=zeros_like(dspacs), UnitX="dSpacing", YUnitLabel="Intensity (a.u.)", OutputWorkspace="ws"
        )
        EditInstrumentGeometry(Workspace=cls.ws, PrimaryFlightPath=50, L2=1, Polar=90)
        cls.comp_func_str = "composite=CompositeFunction,NumDeriv=true;name=Gaussian,Height=125.644,PeakCentre=3.13555,Sigma=0.00317517"

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")  # can be changed by the class
        self.phase.set_hkls([[1, 1, 1]])

    def test_init_no_bg_func(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        self.assertEqual(str(pawley.comp_func), self.comp_func_str)

    def test_init_with_bg_func(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile(), bg_func=FlatBackground(A0=2))
        self.assertEqual(str(pawley.comp_func), ";".join([self.comp_func_str, "name=FlatBackground,A0=2"]))

    def test_get_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        self.assertEqual(len(pawley.get_params()), 7)

    def test_get_free_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        self.assertEqual(len(pawley.get_free_params()), 4)

    def test_get_isfree(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        assert_array_equal(pawley.get_isfree(), array([True, True, True, True, False, False, False]))

    def test_set_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pars = ones(5, dtype=int)
        pawley.set_params(pars)
        assert_array_equal(pawley.get_params().astype(int), pars)

    def test_set_free_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.set_free_params(ones(4))
        assert_array_equal(pawley.get_params().astype(int), array([1, 1, 1, 1, 0, 1, 0]))

    def test_estimate_initial_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.estimate_initial_params()
        self.assertAlmostEqual(pawley.intens[0], 0, delta=1e-2)  # zero intensity as workspace contains only 0

    def test_eval_profile(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        ycalc = pawley.eval_profile(pawley.get_free_params())
        # assert total intensity = 1 (default peak intensity is 1, there is only 1 peak and no background)
        self.assertAlmostEqual(trapezoid(ycalc, self.ws.readX(0)), 1.0, delta=1e-5)

    def test_eval_profile_multiple_phases(self):
        phase2 = Phase.from_alatt(3 * [10.86188], "F d -3 m")  # twice lattice param of silicon
        phase2.set_hkls([[3, 3, 3], [4, 0, 0]])
        pawley = PawleyPattern1D(self.ws, [self.phase, phase2], profile=GaussianProfile())
        ycalc = pawley.eval_profile(pawley.get_free_params())
        # assert total intensity = 3 (three peaks each of intensity 1)
        self.assertAlmostEqual(trapezoid(ycalc, self.ws.readX(0)), 3.0, delta=1e-5)

    def test_eval_resids(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        resids = pawley.eval_resids(pawley.get_free_params())
        # total intensity = 1 and data are 0 so integrated residuals = -1
        self.assertAlmostEqual(trapezoid(resids, self.ws.readX(0)), -1.0, delta=1e-5)

    def test_fit(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        # run fit with 1 func eval to check no error and result returned
        result = pawley.fit(max_nfev=1)
        self.assertEqual(result.nfev, 1)

    def test_inst_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        zero_shift = 0.5
        pawley.inst_params[-1] = zero_shift
        pawley.update_profile_function()
        # assert peak centre shifted by zero_shift
        self.assertAlmostEqual(pawley.comp_func[0]["PeakCentre"], self.phase.calc_dspacings()[0] + zero_shift, delta=1e-8)

    def test_tof_workspace(self):
        ws_tof = ConvertUnits(InputWorkspace=self.ws, OutputWorkspace=self.ws.name() + "_tof", Target="TOF")
        pawley = PawleyPattern1D(ws_tof, [self.phase], profile=GaussianProfile())
        self.assertAlmostEqual(pawley.comp_func[0]["PeakCentre"], 57166, delta=1)

    def test_fit_no_constraints(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        initial_comp_func = str(pawley.comp_func)
        result = pawley.fit_no_constraints(IgnoreInvalidData=False, MaxIterations=2)
        # assert initial parameters changed
        self.assertNotEqual(initial_comp_func, str(result.Function))

    def test_fit_background(self):
        # create data with positive outlier
        x = linspace(0, 1, 4)
        pars = {"A0": 10, "A1": 5}
        func = LinearBackground(**pars)
        y = func(x)
        y[-1] *= 1000  # outlier
        ws_bg_with_outlier = CreateWorkspace(x, y, sqrt(y))

        pawley = PawleyPattern1D(ws_bg_with_outlier, [self.phase], profile=GaussianProfile(), bg_func=func)
        res = pawley.fit_background()

        assert_array_almost_equal(res.x, list(pars.values()), decimal=2)

    # MtdFuncMixin tests


class MtdFuncMixinTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        dspacs = linspace(0.69, 4.15, 2460)
        cls.ws = CreateWorkspace(
            DataX=dspacs, DataY=zeros_like(dspacs), UnitX="dSpacing", YUnitLabel="Intensity (a.u.)", OutputWorkspace="ws"
        )

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        # can init a mixing so init class that inherits from it
        self.phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")  # can be changed by the class
        self.phase.set_hkls([[1, 1, 1], [2, 2, 2]])
        self.pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile(), bg_func=FlatBackground(A0=2))

    def test_get_peak_centers(self):
        assert_array_almost_equal(self.pawley.get_peak_centres(), array([3.14, 1.57]), decimal=2)

    def test_get_peak_params(self):
        assert_array_almost_equal(self.pawley.get_peak_params("Height"), array([125.64, 242.43]), decimal=2)

    def test_get_peak_fwhm(self):
        assert_array_almost_equal(self.pawley.get_peak_fwhm(), array([0.007, 0.004]), decimal=3)

    def test_get_peak_intensities(self):
        assert_array_almost_equal(self.pawley.get_peak_intensities(), ones(2), decimal=3)

    def test_set_mantid_peak_param_isfree(self):
        self.pawley.set_mantid_peak_param_isfree(["Height", "PeakCentre"])
        self.assertEqual(
            str(self.pawley.comp_func),
            "composite=CompositeFunction,NumDeriv=true;name=Gaussian,Height=125.644,PeakCentre=3.13555,Sigma=0.00317517,ties=(Height=125.644,PeakCentre=3.13555);name=Gaussian,Height=242.433,PeakCentre=1.56778,Sigma=0.00164558,ties=(Height=242.433,PeakCentre=1.56778);name=FlatBackground,A0=2",
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
        self.assertFalse(pawley.intens_isfree[0][0])  # fixed so as not to be perfectly correlated with global scale
        self.assertEqual(len(pawley.bg_params), 0)

    def test_set_global_scale_false_with_bg(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile(), bg_func=FlatBackground(A0=2))
        pawley.set_global_scale(False)
        self.assertFalse(pawley.intens_isfree[0][0])  # fixed so as not to be perfectly correlated with global scale
        self.assertTrue(allclose(pawley.bg_params, 0))  # zero global bg (optimised with scale for each spectrum)
        self.assertFalse(pawley.bg_isfree.any(), 0)  # background fixed

    def test_eval_profile(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        ycalc = pawley.eval_profile(pawley.get_free_params())
        # assert total intensity = 1 (default peak intensity is 1, there is only 1 peak and no background)
        self.assertAlmostEqual(trapezoid(ycalc, pawley.ws_1d.readX(0)), self.phase.nhkls(), delta=1e-1)

    def test_eval_resids_global_scale_true(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())
        resids = pawley.eval_resids(pawley.get_free_params())
        self.assertAlmostEqual(sum(resids), -5e7, delta=1e7)

    def test_eval2D_respects_lambda_max(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, lambda_max=5.0, profile=GaussianProfile())
        ysum = pawley.eval_2d(pawley.get_free_params()).extractY().sum()
        pawley.lambda_max = 2.0
        ysum_cropped = pawley.eval_2d(pawley.get_free_params()).extractY().sum()
        self.assertLess(ysum_cropped, ysum)

    def test_eval_resids_global_scale_false(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=False, profile=GaussianProfile())
        resids = pawley.eval_resids(pawley.get_free_params())
        # sum of resids should be much smaller than global-scale=True (as background optimised)
        self.assertAlmostEqual(sum(resids), 2e-10, delta=1e-10)

    @patch("Engineering.pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.pawley_utils.logger")
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

    @patch("Engineering.pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.pawley_utils.logger")
    def test_set_params_from_pawley1d_different_nhkls(self, mock_log, mock_estimate_intens):
        mock_pawley1d = self._make_mock_pawley1d()
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=False, profile=GaussianProfile())
        intial_intens = pawley.intens[0].copy()

        pawley.set_params_from_pawley1d(mock_pawley1d)

        mock_log.error.assert_not_called()
        assert_array_almost_equal(pawley.intens[0], intial_intens)
        assert_array_almost_equal(pawley.profile_params[0], mock_pawley1d.profile_params[0])
        mock_estimate_intens.assert_not_called()

    @patch("Engineering.pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.pawley_utils.logger")
    def test_set_params_from_pawley1d_successful(self, mock_log, mock_estimate_intens):
        mock_pawley1d = self._make_mock_pawley1d()
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
        return mock_pawley1d


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
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=False).create_no_constriants_fit()
        # note intensity on first peak fixed
        assert_array_equal(pawley.get_isfree(), array([False, True, True, True, True, True]))

    def test_global_scale_true_no_bg(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True).create_no_constriants_fit()
        # no intensities fixed
        assert_array_equal(pawley.get_isfree(), ones(6, dtype=bool))

    def test_global_scale_true_with_bg(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True, bg_func=FlatBackground(A0=2)).create_no_constriants_fit()
        # no intensities or bg fixed
        assert_array_equal(pawley.get_isfree(), ones(7, dtype=bool))

    def test_global_scale_false_with_bg(self):
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=False, bg_func=FlatBackground(A0=2)).create_no_constriants_fit()
        # note intensity on first peak fixed and bg fixed
        assert_array_equal(pawley.get_isfree(), array([False, True, True, True, True, True, False]))

    def test_fit(self):
        # run fit with 2 func eval to check no error, result returned and params changed
        pawley = PawleyPattern2D(**self.init_kwargs, global_scale=True).create_no_constriants_fit()
        result = pawley.fit(max_nfev=2)

        self.assertEqual(result.nfev, 2)
        # assert parameters changed
        self.assertFalse(allclose(pawley.initial_params, pawley.get_free_params()))


if __name__ == "__main__":
    unittest.main()
