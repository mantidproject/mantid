import unittest
from numpy import allclose
from mantid.geometry import CrystalStructure
from Engineering.pawley_utils import Phase


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
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], "F d -3 m")
        self._assert_si_phase(si)

    def test_set_hkls_from_dspac_limits(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], "F d -3 m")
        si.set_hkls_from_dspac_limits(2, 3.5)  # (1,1,1) peak only
        self.assertTrue(allclose(si.hkls, 1))
        self.assertEqual(si.nhkls(), 1)

    def test_set_hkls(self):
        si = Phase.from_alatt(3 * [self.SI_LATT_PAR], "F d -3 m")
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


if __name__ == "__main__":
    unittest.main()
