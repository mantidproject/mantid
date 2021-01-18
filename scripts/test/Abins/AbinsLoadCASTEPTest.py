# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import abins.input
import abins.test_helpers
from abins.input import CASTEPLoader


class LoadCASTEPTest(unittest.TestCase, abins.input.Tester):
    # simple tests
    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_castep_reader = CASTEPLoader(input_ab_initio_filename="NonExistingFile.txt")
            bad_castep_reader.read_vibrational_or_phonon_data()

        with self.assertRaises(ValueError):
            # noinspection PyUnusedLocal
            CASTEPLoader(input_ab_initio_filename=1)

    def tearDown(self):
        abins.test_helpers.remove_output_files(list_of_names=["LoadCASTEP"])

    #  *************************** USE CASES ********************************************
    # ===================================================================================
    # | Use case: Gamma point calculation and sum correction enabled during calculations|
    # ===================================================================================
    _gamma_sum = "squaricn_sum_LoadCASTEP"

    def test_gamma_sum_correction(self):
        self.check(name=self._gamma_sum, loader=CASTEPLoader)

    # ===================================================================================
    # |     Use case: Gamma point calculation and no sum correction for Gamma point     |
    # ===================================================================================
    _gamma_no_sum = "squaricn_no_sum_LoadCASTEP"

    def test_gamma_no_sum_correction(self):
        self.check(name=self._gamma_no_sum, loader=CASTEPLoader)

    # ===================================================================================
    # | Use case: more than one k-point and sum correction                              |
    # ===================================================================================
    _many_k_sum = "Si2-phonon_LoadCASTEP"

    def test_sum_correction_single_crystal(self):
        self.check(name=self._many_k_sum, loader=CASTEPLoader)

    # ===================================================================================
    # |   Use case: more than one k-point without sum correction                        |
    # ===================================================================================
    #
    _many_k_no_sum = "Si2-sc_LoadCASTEP"

    def test_no_sum_correction_single_crystal(self):
        self.check(name=self._many_k_no_sum, loader=CASTEPLoader)

    # ===================================================================================
    # |   Use case: system with isotope Li7 and D                                       |
    # ===================================================================================
    #
    _li7_d2 = "LiOH_H2O_7Li_2D2O_LoadCASTEP"

    def test_isotopes(self):
        self.check(name=self._li7_d2, loader=CASTEPLoader)


if __name__ == '__main__':
    unittest.main()
