from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import AbinsModules


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsModules.AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsLoadCASTEPTest because Python is too old.")
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
class AbinsLoadCASTEPTest(unittest.TestCase, AbinsModules.GeneralLoadDFTTester):

    # simple tests
    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_castep_reader = AbinsModules.LoadCASTEP(input_dft_filename="NonExistingFile.txt")
            bad_castep_reader.read_phonon_file()

        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            poor_castep_reader = AbinsModules.LoadCASTEP(input_dft_filename=1)

    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadCASTEP"])


#  *************************** USE CASES ********************************************
# ===================================================================================
    # | Use case: Gamma point calculation and sum correction enabled during calculations|
    # ===================================================================================
    _gamma_sum = "squaricn_sum_LoadCASTEP"

    def test_gamma_sum_correction(self):
        self._check(name=self._gamma_sum, loader=AbinsModules.LoadCASTEP)

    # ===================================================================================
    # |     Use case: Gamma point calculation and no sum correction for Gamma point     |
    # ===================================================================================
    _gamma_no_sum = "squaricn_no_sum_LoadCASTEP"

    def test_gamma_no_sum_correction(self):
        self._check(name=self._gamma_no_sum, loader=AbinsModules.LoadCASTEP)

    # ===================================================================================
    # | Use case: more than one k-point and sum correction       |
    # ===================================================================================
    _many_k_sum = "Si2-phonon_LoadCASTEP"

    def test_sum_correction_single_crystal(self):
        self._check(name=self._many_k_sum, loader=AbinsModules.LoadCASTEP)

    # ===================================================================================
    # |   Use case: more than one k-point without sum correction                        |
    # ===================================================================================
    #
    _many_k_no_sum = "Si2-sc_LoadCASTEP"

    def test_no_sum_correction_single_crystal(self):
        self._check(name=self._many_k_no_sum, loader=AbinsModules.LoadCASTEP)

if __name__ == '__main__':
    unittest.main()
