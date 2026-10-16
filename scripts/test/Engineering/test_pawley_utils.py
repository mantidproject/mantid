# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest.mock import patch, MagicMock
import numpy as np
from numpy import allclose, log, zeros_like, ones, trapezoid, array, linspace, sqrt
from numpy.testing import assert_array_equal, assert_array_almost_equal
from mantid.api import AnalysisDataService, FileFinder
from mantid.simpleapi import (
    CreateWorkspace,
    FlatBackground,
    EditInstrumentGeometry,
    ConvertUnits,
    LinearBackground,
)
from mantid.geometry import CrystalStructure
from mantid.kernel import V3D
from Engineering.pawley_utils import Phase, GaussianProfile, PVProfile, PawleyPattern1D, BackToBackGauss
from Engineering.poldi_pawley_utils import PawleyPattern2D
from plugins.algorithms.poldi_utils import load_poldi


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

    def test_from_phase_copies_phase(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR, "", "Si", ",")
        si.set_hkls_from_dspac_limits(2, 3.5)  # (1,1,1) peak only
        si_copy = Phase.from_phase(si)
        self._assert_si_phase(si_copy)
        self.assertEqual([Phase.hkl_as_key(hkl) for hkl in si_copy.hkls], [Phase.hkl_as_key(hkl) for hkl in si.hkls])

    def test_from_phase_copy_is_independent_of_original(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR, "", "Si", ",")
        si.set_hkls_from_dspac_limits(2, 3.5)  # (1,1,1) peak only
        si_copy = Phase.from_phase(si)
        si_copy.set_params([1.1 * self.SI_LATT_PAR])
        si_copy.set_hkls([[2, 2, 0]])
        self.assertAlmostEqual(si.get_params()[0], self.SI_LATT_PAR, delta=1e-5)
        self.assertTrue(allclose(si.hkls, 1))

    def test_from_phase_without_hkls(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR, "", "Si", ",")
        si.set_hkls_from_dspac_limits(2, 3.5)
        si_copy = Phase.from_phase(si, copy_hkls=False)
        self.assertEqual(si_copy.nhkls(), 0)

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

    def test_set_get_profile_by_name(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.set_profile_param(2.2, "sig0", 0)
        self.assertEqual(pawley.get_profile_param("sig0", 0), 2.2)
        assert_array_equal(pawley.get_params().astype(int), array([5, 1, 2, 0, 0, 1, 0]))

    def test_set_get_phase_by_name(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.set_phase_param(4.65, "a", 0)
        self.assertEqual(pawley.get_phase_param("a", 0), 4.65)
        self.assertEqual(pawley.get_params()[0], 4.65)

    def test_set_get_phase_free_by_name(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.set_phase_free_param(False, "a", 0)
        self.assertEqual(pawley.get_phase_free_param("a", 0), False)
        self.assertFalse(pawley.get_isfree()[0])

    def test_set_get_profile_free_param_by_name(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        assert_array_equal(pawley.get_isfree(), array([True, True, True, True, False, False, False]))
        pawley.set_profile_free_param(True, "sig2", 0)
        self.assertEqual(pawley.get_profile_free_param("sig2", 0), True)
        assert_array_equal(pawley.get_isfree(), array([True, True, True, True, True, False, False]))

    def test_profile_param_by_name_validator(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        self.assertRaisesRegex(
            ValueError, "Invalid profile parameter name: alpha, for profile Gaussian", pawley.get_profile_param, "alpha", 0
        )
        self.assertRaisesRegex(
            ValueError, "Phase with index: 3 does not exists. Number of phases is 1", pawley.get_profile_param, "sig0", 3
        )

    def test_estimate_initial_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.estimate_initial_params()
        self.assertAlmostEqual(pawley.intens[0], 0, delta=1e-2)  # zero intensity as workspace contains only 0

    def test_eval_profile(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        ycalc = pawley.eval_profile(pawley.get_free_params())
        # assert total intensity = 1 (default peak intensity is 1, there is only 1 peak and no background)
        self.assertAlmostEqual(trapezoid(ycalc, self.ws.x(0)), 1.0, delta=1e-5)

    def test_eval_profile_multiple_phases(self):
        phase2 = Phase.from_alatt(3 * [10.86188], "F d -3 m")  # twice lattice param of silicon
        phase2.set_hkls([[3, 3, 3], [4, 0, 0]])
        pawley = PawleyPattern1D(self.ws, [self.phase, phase2], profile=GaussianProfile())
        ycalc = pawley.eval_profile(pawley.get_free_params())
        # assert total intensity = 3 (three peaks each of intensity 1)
        self.assertAlmostEqual(trapezoid(ycalc, self.ws.x(0)), 3.0, delta=1e-5)

    def test_eval_resids(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        resids = pawley.eval_resids(pawley.get_free_params())
        # total intensity = 1 and data are 0 so integrated residuals = -1
        self.assertAlmostEqual(trapezoid(resids, self.ws.x(0)), -1.0, delta=1e-5)

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

    def test_name_and_delimiter_propagated(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR, hkl_delimiter=",")
        phase.set_hkls_from_dspac_limits(0.7, 3.5)
        phase.set_phase_name("Silicon")
        filtered = phase.filter_hkls_to_ws_range(self.ws)
        self.assertEqual(filtered.get_phase_name(), "Silicon")
        self.assertEqual(filtered.hkl_str_delimiter, ",")

    def test_original_phase_not_modified(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        phase.set_hkls_from_dspac_limits(0.4, 5.0)
        hkls_before = [Phase.hkl_as_key(hkl) for hkl in phase.hkls]
        phase.filter_hkls_to_ws_range(self.ws)
        self.assertEqual([Phase.hkl_as_key(hkl) for hkl in phase.hkls], hkls_before)

    def test_filtered_hkls_are_subset_of_original(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        phase.set_hkls_from_dspac_limits(0.4, 5.0)
        hkls_before = [Phase.hkl_as_key(hkl) for hkl in phase.hkls]
        filtered = phase.filter_hkls_to_ws_range(self.ws)
        hkls_after = [Phase.hkl_as_key(hkl) for hkl in filtered.hkls]
        # only removes reflections - never introduces new ones or reorders those retained
        self.assertTrue(set(hkls_after).issubset(set(hkls_before)))
        self.assertEqual(hkls_after, [hkl for hkl in hkls_before if hkl in set(hkls_after)])

    def test_merged_reflections_not_reintroduced_by_filter(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        phase.set_hkls_from_dspac_limits(0.7, 3.5)
        phase.merge_reflections()  # drops e.g. (5,1,1), degenerate in d with (3,3,3)
        merged_hkls = set(Phase.hkl_as_key(hkl) for hkl in phase.hkls)
        self.assertNotIn((5, 1, 1), merged_hkls)
        filtered = phase.filter_hkls_to_ws_range(self.ws)
        self.assertNotIn((5, 1, 1), set(Phase.hkl_as_key(hkl) for hkl in filtered.hkls))

    def test_set_hkls_from_ws_range_modifies_in_place(self):
        phase = Phase.from_alatt(3 * [self.SI_LATT_PAR], self.SI_SPGR)
        phase.set_hkls_from_dspac_limits(0.4, 5.0)
        nhkls_before = phase.nhkls()
        phase.set_hkls_from_ws_range(self.ws)
        self.assertGreater(phase.nhkls(), 0)
        self.assertLess(phase.nhkls(), nhkls_before)


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


if __name__ == "__main__":
    unittest.main()
