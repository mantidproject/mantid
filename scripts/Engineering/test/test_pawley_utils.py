import unittest
from numpy import allclose, sqrt, log, linspace, zeros_like, array, ones, trapezoid
from numpy.testing import assert_array_equal
from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateWorkspace, FlatBackground
from mantid.geometry import CrystalStructure
from Engineering.pawley_utils import Phase, GaussianProfile, PVProfile, PawleyPattern1D


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

    def _assert_si_phase(self, phase):
        pars = phase.get_params()
        self.assertEqual(len(pars), 1)  # only 1 lattice parameter in cubic system (a)
        self.assertAlmostEqual(pars[0], self.SI_LATT_PAR, delta=1e-5)
        self.assertEqual(phase.spgr.getHMSymbol(), self.SI_SPGR)
        self.assertListEqual(phase.get_param_names(), ["a"])
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


class PawleyPattern1DTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        dspacs = linspace(0.69, 4.15, 2460)
        cls.ws = CreateWorkspace(
            DataX=dspacs, DataY=zeros_like(dspacs), UnitX="dSpacing", YUnitLabel="Intensity (a.u.)", OutputWorkspace="ws"
        )
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
        self.assertEqual(len(pawley.get_params()), 5)

    def test_get_free_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        self.assertEqual(len(pawley.get_free_params()), 4)

    def test_get_isfree(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        assert_array_equal(pawley.get_isfree(), array([True, True, True, True, False]))

    def test_set_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pars = ones(5, dtype=int)
        pawley.set_params(pars)
        assert_array_equal(pawley.get_params().astype(int), pars)

    def test_set_free_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.set_free_params(ones(4))
        assert_array_equal(pawley.get_params().astype(int), array([1, 1, 1, 1, 0]))

    def test_estimate_initial_params(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        pawley.estimate_initial_params()
        self.assertAlmostEqual(pawley.intens[0], 0, delta=1e-2)  # zero intensity as workspace contains only 0

    def test_eval_profile(self):
        pawley = PawleyPattern1D(self.ws, [self.phase], profile=GaussianProfile())
        ycalc = pawley.eval_profile(pawley.get_free_params())
        # assert total intensity = 1 (default peak intensity is 1, there is only 1 peak and no background)
        self.assertAlmostEqual(trapezoid(ycalc, self.ws.readX(0)), 1.0, delta=1e-5)

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


if __name__ == "__main__":
    unittest.main()
