# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import abins.test_helpers
import abins.input
from abins.input import CRYSTALLoader


class AbinsLoadCRYSTALTest(unittest.TestCase, abins.input.Tester):
    def tearDown(self):
        from mantid.kernel import ConfigService

        abins.test_helpers.remove_output_files(list_of_names=["_LoadCRYSTAL"], directory=ConfigService.getString("defaultsave.directory"))

    # *************************** USE CASES *********************************************
    # ===================================================================================
    # | Use cases: Gamma point calculation for CRYSTAL                                  |
    # ===================================================================================
    _gamma_crystal = "crystalB3LYP_LoadCRYSTAL"
    _set_crystal = "crystal_set_key_LoadCRYSTAL"

    # ===================================================================================
    # | Use case: Molecular calculation for CRYSTAL                                     |
    # ===================================================================================
    _molecule = "toluene_molecule_LoadCRYSTAL"

    # ===================================================================================
    # | Use case: Molecular calculation with CRYSTAL17                                  |
    # ===================================================================================
    _molecule17 = "toluene_molecule_LoadCRYSTAL17"

    # ===================================================================================
    # | Use cases: Phonon dispersion calculation for CRYSTAL                            |
    # ===================================================================================
    _phonon_dispersion_v1 = "mgo-GX_LoadCRYSTAL"
    _phonon_dispersion_v2 = "MgO-222-DISP_LoadCRYSTAL"

    # ===================================================================================
    # | Use cases: Interpolated phonon DOS                                              |
    # ===================================================================================
    _phonon_dos = "crystal23_diamond"

    def test_gamma_crystal(self):
        self.check(name=self._gamma_crystal, loader=CRYSTALLoader)
        self.check(name=self._set_crystal, loader=CRYSTALLoader)

    def test_molecule(self):
        self.check(name=self._molecule, loader=CRYSTALLoader)

    def test_molecule17(self):
        self.check(name=self._molecule17, loader=CRYSTALLoader)

    def test_phonon_dispersion_crystal(self):
        self.check(name=self._phonon_dispersion_v1, loader=CRYSTALLoader)
        self.check(name=self._phonon_dispersion_v2, loader=CRYSTALLoader)

    def test_phonon_dos_crystal(self):
        self.check(name=self._phonon_dos, loader=CRYSTALLoader, max_displacement_kpt=3)


if __name__ == "__main__":
    unittest.main()
