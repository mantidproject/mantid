# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch, create_autospec, MagicMock
import numpy as np
from numpy import allclose, log, zeros_like, ones, trapezoid, array, linspace, sqrt
from numpy.testing import assert_array_equal, assert_array_almost_equal
from mantid.api import AnalysisDataService, FileFinder
from mantid.simpleapi import CreateWorkspace, FlatBackground, EditInstrumentGeometry, ConvertUnits, LinearBackground
from mantid.geometry import CrystalStructure
from mantid.kernel import V3D
from Engineering.pawley_utils import Phase, GaussianProfile, PVProfile, PawleyPattern1D, PawleyPattern2D, BackToBackGauss
from plugins.algorithms.poldi_utils import load_poldi, _do_interp_with_flux_correction, _get_flux_arrays, simulate_2d_data


class PhaseTest(unittest.TestCase):
    SI_LATT_PAR = 5.43094
    SI_SPGR = "F d -3 m"

    def test_init(self):
        si = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""), None, "Si", ",")
        self._assert_si_phase(si)

    def test_init_phase_from_cif(self):
        si = Phase.from_cif("SI.cif", "Si", ",")
        self._assert_si_phase(si)

    def test_init_phase_from_alatt(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR, "", "Si", ",")
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
        self.assertEqual(phase.name, "Si")
        self.assertEqual(phase.hkl_str_delimiter, ",")

    # --- hkl_as_key ---

    def test_hkl_as_key_returns_int_tuple(self):
        hkl = V3D(1, 2, 3)
        result = Phase.hkl_as_key(hkl)
        self.assertEqual(result, (1, 2, 3))
        self.assertIsInstance(result, tuple)
        self.assertTrue(all(isinstance(v, int) for v in result))

    def test_hkl_as_key_rounds_float_values(self):
        hkl = V3D(1.6, 2.4, 3.5)
        result = Phase.hkl_as_key(hkl)
        self.assertEqual(result, (2, 2, 4))

    # --- get_hkl_strings ---

    def test_get_hkl_strings_single_digit(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        phase.set_hkls([[1, 1, 1]])
        self.assertEqual(phase.get_hkl_strings(), ["111"])

    def test_get_hkl_strings_multiple_hkls(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        phase.set_hkls([[1, 1, 1], [2, 2, 0]])
        result = phase.get_hkl_strings()
        self.assertEqual(result, ["111", "220"])

    def test_get_hkl_strings_multi_digit_indices(self):
        # use a space group that allows all reflections
        # set delimiter to distinguish indices
        phase = Phase.from_alatt([10.0, 10.0, 10.0], "P 1", hkl_delimiter="-")
        phase.set_hkls([[10, 11, 12]])
        self.assertEqual(phase.get_hkl_strings(), ["10-11-12"])

    # --- get_hkl_strings ---

    def test_get_hkl_from_str_default_delim(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        self.assertEqual(phase.hkl_string_to_indices("111"), [1, 1, 1])

    def test_get_hkl_from_str_provided_delim_on_phase(self):
        delim = "-"
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""), None, "", delim)
        self.assertEqual(phase.hkl_string_to_indices("1-1-1"), [1, 1, 1])

    def test_get_hkl_from_str_provided_delim_in_method(self):
        delim = "-"
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        self.assertEqual(phase.hkl_string_to_indices("1-1-1", delim), [1, 1, 1])

    @patch("Engineering.pawley_utils.logger")
    def test_get_hkl_from_str_using_default_delim_but_input_has_different_delim(self, mock_logger):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        hkl_inds = phase.hkl_string_to_indices("1-1-1")
        self.assertEqual(hkl_inds, None)
        mock_logger.error.assert_called_once_with("Can't convert input string ('1-1-1') to H,K, and L indices. Delimiter being used is: ''")

    @patch("Engineering.pawley_utils.logger")
    def test_get_hkl_from_str_with_bad_number_of_indices(self, mock_logger):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        hkl_inds = phase.hkl_string_to_indices("1-1")
        self.assertEqual(hkl_inds, None)
        mock_logger.error.assert_called_once_with("Can't convert input string ('1-1') to H,K, and L indices. Delimiter being used is: ''")

    # --- get_phase_name / set_phase_name ---

    def test_get_phase_name_default_is_none(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        self.assertIsNone(phase.get_phase_name())

    def test_set_phase_name(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        phase.set_phase_name("Silicon")
        self.assertEqual(phase.get_phase_name(), "Silicon")

    def test_set_phase_name_overwrite(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        phase.set_phase_name("Alpha")
        phase.set_phase_name("Beta")
        self.assertEqual(phase.get_phase_name(), "Beta")

    # --- has_the_same_parameters_as ---

    def test_has_the_same_parameters_as_same_spacegroup(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        other = Phase.from_alatt(3 * [4.0], self.SI_SPGR)
        self.assertTrue(phase.has_the_same_parameters_as(other))

    def test_has_the_same_parameters_as_different_lattice_system(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        tetragonal = Phase.from_alatt(3 * [self.SI_LATT_PAR], "P 4")
        self.assertFalse(phase.has_the_same_parameters_as(tetragonal))

    def test_has_the_same_parameters_as_same_lattice_system_different_spacegroup(self):
        phase = Phase(CrystalStructure(f"{self.SI_LATT_PAR} {self.SI_LATT_PAR} {self.SI_LATT_PAR}", self.SI_SPGR, ""))
        other_cubic = Phase.from_alatt(3 * [4.0], "P m -3 m")
        self.assertTrue(phase.has_the_same_parameters_as(other_cubic))


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
        EditInstrumentGeometry(Workspace=ws_bg_with_outlier, PrimaryFlightPath=50, L2=1, Polar=90)

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
        self.assertEqual(len(pawley.bg_params), 0)

    def test_set_global_scale_false_with_bg(self):
        pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile(), bg_func=FlatBackground(A0=2))
        pawley.set_global_scale(False)
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
        mock_pawley1d.phases[0].get_param_names.return_value = self.phase.get_param_names()
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

    @patch("Engineering.pawley_utils.PawleyPattern2D._estimate_intensities")
    @patch("Engineering.pawley_utils.logger")
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
    @patch("Engineering.pawley_utils.least_squares")
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
    @patch("Engineering.pawley_utils.least_squares")
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
    @patch("Engineering.pawley_utils.least_squares")
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


class OutputTableMixinTest(unittest.TestCase):
    """Tests for OutputTableMixin.create_output_tables, exercised through PawleyPattern1D."""

    LATT_SI = 5.43094
    SPGR_SI = "F d -3 m"

    @classmethod
    def setUpClass(cls):
        dspacs = linspace(0.69, 4.15, 2460)
        cls.ws = CreateWorkspace(
            DataX=dspacs,
            DataY=zeros_like(dspacs),
            UnitX="dSpacing",
            YUnitLabel="Intensity (a.u.)",
            OutputWorkspace="ws_tab_mixin",
        )
        EditInstrumentGeometry(Workspace=cls.ws, PrimaryFlightPath=50, L2=1, Polar=90)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def setUp(self):
        self.phase = Phase.from_alatt(3 * [self.LATT_SI], self.SPGR_SI)
        self.phase.set_hkls([[1, 1, 1]])
        self.phase2 = Phase.from_alatt(3 * [self.LATT_SI], self.SPGR_SI)
        self.phase2.set_hkls([[2, 2, 2]])

    def _pawley(self, phases=None):
        return PawleyPattern1D(self.ws, phases or [self.phase], profile=GaussianProfile())

    def _fit(self, pawley):
        pawley.fit(max_nfev=1)

    # --- return type and length ---

    def test_single_phase_returns_list_of_one_table(self):
        pawley = self._pawley()
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertIsInstance(tables, list)
        self.assertEqual(len(tables), 1)

    def test_two_phases_returns_list_of_two_tables(self):
        pawley = self._pawley([self.phase, self.phase2])
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(len(tables), 2)

    # --- workspace naming ---

    def test_default_workspace_name_uses_phase_index(self):
        pawley = self._pawley()
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].name(), f"{self.ws.name()}_pawley_table_phase0")

    def test_two_phases_default_names_use_index(self):
        pawley = self._pawley([self.phase, self.phase2])
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].name(), f"{self.ws.name()}_pawley_table_phase0")
        self.assertEqual(tables[1].name(), f"{self.ws.name()}_pawley_table_phase1")

    def test_phase_names_used_in_workspace_names(self):
        phases = [self.phase, self.phase2]
        for iphase, phase_name in enumerate(["alpha", "beta"]):
            phases[iphase].set_phase_name(phase_name)
        pawley = self._pawley([self.phase, self.phase2])
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].name(), f"{self.ws.name()}_pawley_table_alpha")
        self.assertEqual(tables[1].name(), f"{self.ws.name()}_pawley_table_beta")

    def test_custom_base_name_prepended_to_phase_name(self):
        self.phase.set_phase_name("Si")
        pawley = self._pawley([self.phase])
        self._fit(pawley)
        tables = pawley.create_output_tables(output_workspace="my_fit")
        self.assertEqual(tables[0].name(), "my_fit_Si")

    # --- table structure ---

    def test_table_has_required_columns(self):
        pawley = self._pawley()
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(
            tables[0].getColumnNames(),
            ["Workspace", "Spectrum", "HKL", "I", "I_err", "X0", "X0_err", "FWHM", "FWHM_err"],
        )

    def test_row_count_matches_phase_nhkls(self):
        pawley = self._pawley()
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].rowCount(), self.phase.nhkls())

    def test_hkl_column_contains_phase_hkl_strings(self):
        pawley = self._pawley()
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].column("HKL"), self.phase.get_hkl_strings())

    # --- table per phase ---

    def test_two_phases_each_table_row_count_matches_its_phase(self):
        pawley = self._pawley([self.phase, self.phase2])
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].rowCount(), self.phase.nhkls())
        self.assertEqual(tables[1].rowCount(), self.phase2.nhkls())

    def test_two_phases_each_table_contains_only_its_phase_hkls(self):
        pawley = self._pawley([self.phase, self.phase2])
        self._fit(pawley)
        tables = pawley.create_output_tables()
        self.assertEqual(tables[0].column("HKL"), self.phase.get_hkl_strings())
        self.assertEqual(tables[1].column("HKL"), self.phase2.get_hkl_strings())

    def test_two_phases_hkl_sets_are_different(self):
        pawley = self._pawley([self.phase, self.phase2])
        self._fit(pawley)
        tables = pawley.create_output_tables()
        hkls_0 = set(tables[0].column("HKL"))
        hkls_1 = set(tables[1].column("HKL"))
        self.assertTrue(hkls_0.isdisjoint(hkls_1))


class PhaseFilterHklsTest(unittest.TestCase):
    """Tests for Phase.filter_hkls_to_ws_range using a POLDI workspace."""

    SI_LATT_PAR = 5.43094
    SI_SPGR = "F d -3 m"

    @classmethod
    def setUpClass(cls):
        fpath_data = FileFinder.getFullPath("poldi_448x500_chopper5k_silicon.txt")
        cls.ws = load_poldi(fpath_data, "POLDI_Definition_448_calibrated.xml", chopper_speed=5000, t0=5.855e-02, t0_const=-9.00)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def test_all_hkls_in_range(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        phase.set_hkls_from_dspac_limits(0.7, 3.5)
        nhkls_before = phase.nhkls()
        filtered = phase.filter_hkls_to_ws_range(self.ws)
        # all peaks should be within the POLDI angular range
        self.assertEqual(filtered.nhkls(), nhkls_before)

    def test_subset_in_range(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        # set d-spacing range wider than POLDI can access so some are filtered out
        phase.set_hkls_from_dspac_limits(0.4, 5.0)
        nhkls_before = phase.nhkls()
        filtered = phase.filter_hkls_to_ws_range(self.ws)
        self.assertGreater(filtered.nhkls(), 0)
        self.assertLess(filtered.nhkls(), nhkls_before)

    def test_none_in_range(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        # only very high d-spacing HKL (111) — restrict lambda range so it's out of range
        phase.set_hkls([[1, 1, 1]])
        filtered = phase.filter_hkls_to_ws_range(self.ws, lambda_min=5.1, lambda_max=5.2)
        self.assertEqual(filtered.hkls, [])

    def test_name_propagated(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        phase.set_hkls_from_dspac_limits(0.7, 3.5)
        phase.set_phase_name("Silicon")
        filtered = phase.filter_hkls_to_ws_range(self.ws)
        self.assertEqual(filtered.get_phase_name(), "Silicon")


class BoundsMixinTest(unittest.TestCase):
    """Tests for BoundsMixin methods via PawleyPattern2D."""

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
        self.pawley = PawleyPattern2D(self.ws, [self.phase], global_scale=True, profile=GaussianProfile())

    # --- set_bounds mode="log" ---

    def test_set_bounds_log_positive_param(self):
        name = self.pawley.get_param_names()[0]
        param_val = self.pawley.get_params()[0]
        factor = 2.0
        self.pawley.set_bounds(names=name, mode="log", values=factor)
        lb, ub = self.pawley._param_bounds[name]
        self.assertAlmostEqual(lb, param_val / factor)
        self.assertAlmostEqual(ub, param_val * factor)

    def test_set_bounds_log_negative_param(self):
        # force a negative parameter value to test log bounds for negative
        names = self.pawley.get_param_names()
        name = names[0]
        params = self.pawley.get_params()
        params[0] = -5.0
        self.pawley.set_params(params)
        factor = 2.0
        self.pawley.set_bounds(names=name, mode="log", values=factor)
        lb, ub = self.pawley._param_bounds[name]
        # for negative p: lb = p * factor, ub = p / factor
        self.assertAlmostEqual(lb, -5.0 * factor)
        self.assertAlmostEqual(ub, -5.0 / factor)

    def test_set_bounds_log_zero_param(self):
        names = self.pawley.get_param_names()
        name = names[0]
        params = self.pawley.get_params()
        params[0] = 0.0
        self.pawley.set_params(params)
        self.pawley.set_bounds(names=name, mode="log", values=2.0)
        lb, ub = self.pawley._param_bounds[name]
        self.assertAlmostEqual(lb, -self.pawley.param_bounds_abs_min)
        self.assertAlmostEqual(ub, self.pawley.param_bounds_abs_min)

    # --- set_bounds mode="absolute" ---

    def test_set_bounds_absolute(self):
        name = self.pawley.get_param_names()[0]
        self.pawley.set_bounds(names=name, mode="absolute", lb=-10.0, ub=20.0)
        stored_lb, stored_ub = self.pawley._param_bounds[name]
        self.assertAlmostEqual(stored_lb, -10.0)
        self.assertAlmostEqual(stored_ub, 20.0)

    def test_set_bounds_absolute_lb_only(self):
        name = self.pawley.get_param_names()[0]
        self.pawley.set_bounds(names=name, mode="absolute", lb=0.0)
        stored_lb, stored_ub = self.pawley._param_bounds[name]
        self.assertAlmostEqual(stored_lb, 0.0)
        self.assertEqual(stored_ub, np.inf)

    # --- set_bounds mode=None (removal) ---

    def test_set_bounds_mode_none_removes_bounds(self):
        name = self.pawley.get_param_names()[0]
        self.pawley.set_bounds(names=name, mode="fractional", values=0.1)
        self.assertIn(name, self.pawley._param_bounds)
        self.pawley.set_bounds(names=name, mode=None)
        self.assertNotIn(name, self.pawley._param_bounds)

    # --- clear_bounds ---

    def test_clear_bounds_all(self):
        self.pawley.set_bounds_all(mode="fractional", value=0.1)
        self.assertTrue(len(self.pawley._param_bounds) > 0)
        self.pawley.clear_bounds()
        self.assertEqual(len(self.pawley._param_bounds), 0)

    def test_clear_bounds_specific_names(self):
        names = list(self.pawley.get_param_names())
        self.pawley.set_bounds_all(mode="fractional", value=0.1)
        self.pawley.clear_bounds(names[0])
        self.assertNotIn(names[0], self.pawley._param_bounds)
        # other bounds should remain
        self.assertIn(names[1], self.pawley._param_bounds)

    def test_clear_bounds_single_string(self):
        name = self.pawley.get_param_names()[0]
        self.pawley.set_bounds(names=name, mode="fractional", values=0.1)
        self.pawley.clear_bounds(name)
        self.assertNotIn(name, self.pawley._param_bounds)

    # --- set_non_negative ---

    def test_set_non_negative_no_existing_bounds(self):
        name = self.pawley.get_param_names()[0]
        self.pawley.set_non_negative(name)
        lb, ub = self.pawley._param_bounds[name]
        self.assertAlmostEqual(lb, 0.0)
        self.assertEqual(ub, np.inf)

    def test_set_non_negative_with_existing_bounds(self):
        name = self.pawley.get_param_names()[0]
        self.pawley.set_bounds(names=name, mode="absolute", lb=-5.0, ub=10.0)
        self.pawley.set_non_negative(name)
        lb, ub = self.pawley._param_bounds[name]
        self.assertAlmostEqual(lb, 0.0)
        self.assertAlmostEqual(ub, 10.0)

    def test_set_non_negative_with_positive_existing_lb(self):
        name = self.pawley.get_param_names()[0]
        self.pawley.set_bounds(names=name, mode="absolute", lb=2.0, ub=10.0)
        self.pawley.set_non_negative(name)
        lb, ub = self.pawley._param_bounds[name]
        # lb should stay at 2.0 since max(2.0, 0.0) = 2.0
        self.assertAlmostEqual(lb, 2.0)
        self.assertAlmostEqual(ub, 10.0)

    # --- repeat_values ---

    def test_repeat_values_scalar_repeated(self):
        from Engineering.pawley_utils import BoundsMixin

        result = BoundsMixin.repeat_values(0.1, 3, "test")
        self.assertEqual(result, [0.1, 0.1, 0.1])

    def test_repeat_values_matching_list_returned(self):
        from Engineering.pawley_utils import BoundsMixin

        values = [1.0, 2.0, 3.0]
        result = BoundsMixin.repeat_values(values, 3, "test")
        self.assertEqual(result, values)

    def test_repeat_values_mismatched_length_raises(self):
        from Engineering.pawley_utils import BoundsMixin

        with self.assertRaises(ValueError):
            BoundsMixin.repeat_values([1.0, 2.0], 3, "myarg")


class OutputTableAdditionalTest(unittest.TestCase):
    """Additional tests for OutputTableMixin: I_err populated and ill-conditioned Hessian."""

    @classmethod
    def setUpClass(cls):
        # create a workspace with a real peak so fitting produces meaningful errors
        dspacs = linspace(0.69, 4.15, 2460)
        sigma = 0.003
        peak_centre = 3.13555
        y = 100 * np.exp(-0.5 * ((dspacs - peak_centre) / sigma) ** 2)
        cls.ws = CreateWorkspace(
            DataX=dspacs,
            DataY=y,
            DataE=sqrt(np.maximum(y, 1.0)),
            UnitX="dSpacing",
            OutputWorkspace="ws_output_tab",
        )
        EditInstrumentGeometry(Workspace=cls.ws, PrimaryFlightPath=50, L2=1, Polar=90)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def test_I_err_populated_after_real_fit(self):
        phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")
        phase.set_hkls([[1, 1, 1]])
        pawley = PawleyPattern1D(self.ws, [phase], profile=GaussianProfile())
        pawley.fit(max_nfev=50)
        tables = pawley.create_output_tables()
        i_err = tables[0].column("I_err")[0]
        self.assertFalse(np.isnan(i_err), "I_err should not be NaN after a real fit")
        self.assertGreater(i_err, 0, "I_err should be positive after fitting real data")

    @patch("Engineering.pawley_utils.logger")
    def test_get_parameter_errors_ill_conditioned_hessian(self, mock_log):
        from Engineering.pawley_utils import OutputTableMixin

        # create a mock result with a near-singular Jacobian
        mock_res = MagicMock()
        # make J such that J^T J is ill-conditioned
        mock_res.jac = np.array([[1.0, 1.0], [1.0, 1.0 + 1e-15], [0.0, 0.0]])
        mock_res.fun = np.array([0.1, 0.2, 0.3])
        errors = OutputTableMixin.get_parameter_errors(mock_res)
        mock_log.warning.assert_called_once()
        self.assertEqual(len(errors), 2)
        # errors should be finite (pseudoinverse used)
        self.assertTrue(np.all(np.isfinite(errors)))


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


class PawleyPattern1DSetIspecTest(unittest.TestCase):
    """Tests for PawleyPattern1D.set_ispec."""

    @classmethod
    def setUpClass(cls):
        dspacs = linspace(0.69, 4.15, 2460)
        cls.ws = CreateWorkspace(DataX=dspacs, DataY=zeros_like(dspacs), UnitX="dSpacing", OutputWorkspace="ws_set_ispec")
        EditInstrumentGeometry(Workspace=cls.ws, PrimaryFlightPath=50, L2=1, Polar=90)

    @classmethod
    def tearDownClass(cls):
        AnalysisDataService.clear()

    def test_set_ispec_updates_ispec(self):
        phase = Phase.from_alatt(3 * [5.43094], "F d -3 m")
        phase.set_hkls([[1, 1, 1]])
        pawley = PawleyPattern1D(self.ws, [phase], profile=GaussianProfile())
        pawley.set_ispec(0)
        self.assertEqual(pawley.ispec, 0)


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
        mock_interp.return_value = zeros_like(ws_2d.readX(0))

        simulate_2d_data(ws_2d, ws_1d, flux_sample_points=None)

        mock_flux_interp.assert_not_called()
        self.assertGreater(mock_interp.call_count, 0)
        AnalysisDataService.clear()


if __name__ == "__main__":
    unittest.main()
