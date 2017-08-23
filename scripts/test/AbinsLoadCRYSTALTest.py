from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import AbinsModules


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsModules.AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsLoadCRYSTALTest because Python is too old.")
    return is_python_old


def skip_if(skipping_criteria):
    """
    Skip all tests if the supplied function returns true.
    Python unittest.skipIf is not available in 2.6 (RHEL6) so we'll roll our own.
    """
    def decorate(cls):
        if skipping_criteria():
            for attr in cls.__dict__.keys():
                if callable(getattr(cls, attr)) and 'test' in attr:
                    delattr(cls, attr)
        return cls
    return decorate


@skip_if(old_python)
class AbinsLoadCRYSTALTest(unittest.TestCase, AbinsModules.GeneralLoadDFTTester):

    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadCRYSTAL"])

        #  *************************** USE CASES ********************************************
    # ===================================================================================
    # | Use cases: Gamma point calculation for CRYSTAL                                    |
    # ===================================================================================
    _gamma_crystal = "crystalB3LYP_LoadCRYSTAL"
    _set_crystal = "crystal_set_key_LoadCRYSTAL"

    # ===================================================================================
    # | Use case: Molecular calculation for CRYSTAL                                     |
    # ===================================================================================
    _molecule = "toluene_molecule_LoadCRYSTAL"

    # ===================================================================================
    # | Use cases: Phonon dispersion calculation for CRYSTAL                                     |
    # ===================================================================================
    _phonon_dispersion_v1 = "mgo-GX_LoadCRYSTAL"
    _phonon_dispersion_v2 = "MgO-222-DISP_LoadCRYSTAL"

    def test_gamma_crystal(self):
        self._check(name=self._gamma_crystal, loader=AbinsModules.LoadCRYSTAL)
        self._check(name=self._set_crystal, loader=AbinsModules.LoadCRYSTAL)

    def test_molecule(self):
        self._check(name=self._molecule, loader=AbinsModules.LoadCRYSTAL)

    def test_phonon_dispersion_crystal(self):
        self._check(name=self._phonon_dispersion_v1, loader=AbinsModules.LoadCRYSTAL)
        self._check(name=self._phonon_dispersion_v2, loader=AbinsModules.LoadCRYSTAL)

if __name__ == '__main__':
    unittest.main()
