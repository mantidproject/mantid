# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import AbinsModules


class AbinsLoadCRYSTALTest(unittest.TestCase, AbinsModules.GeneralLoadAbInitioTester):

    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadCRYSTAL"])

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

    def test_gamma_crystal(self):
        self.check(name=self._gamma_crystal, loader=AbinsModules.LoadCRYSTAL)
        self.check(name=self._set_crystal, loader=AbinsModules.LoadCRYSTAL)

    def test_molecule(self):
        self.check(name=self._molecule, loader=AbinsModules.LoadCRYSTAL)

    def test_molecule17(self):
        self.check(name=self._molecule17, loader=AbinsModules.LoadCRYSTAL)

    def test_phonon_dispersion_crystal(self):
        self.check(name=self._phonon_dispersion_v1, loader=AbinsModules.LoadCRYSTAL)
        self.check(name=self._phonon_dispersion_v2, loader=AbinsModules.LoadCRYSTAL)


if __name__ == '__main__':
    unittest.main()
