# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch, Mock

import abins.input
import abins.test_helpers
from abins.input import CASTEPLoader


class LoadCastepUsingEuphonicTest(unittest.TestCase):
    from euphonic import QpointPhononModes

    from_castep = Mock(wraps=QpointPhononModes.from_castep)

    @patch("euphonic.QpointPhononModes.from_castep")
    def test_euphonic_castep_call(self, from_castep):
        # We mostly trust Euphonic to read the file, so just check the right
        # filename makes it to Euphonic CASTEP parser

        filename = abins.test_helpers.find_file("squaricn_sum_LoadCASTEP.phonon")

        with TemporaryDirectory() as tmpdir:
            reader = CASTEPLoader(input_ab_initio_filename=filename, cache_directory=Path(tmpdir))

            try:
                reader.read_vibrational_or_phonon_data()
            except TypeError:
                pass  # Clerk will freak out when passed mocked data to serialise

        from_castep.assert_called_with(filename, prefer_non_loto=True)


class LoadCASTEPTest(unittest.TestCase, abins.input.Tester):
    # simple tests
    def test_non_existing_file(self):
        with self.assertRaises(IOError):
            bad_castep_reader = CASTEPLoader(input_ab_initio_filename="NonExistingFile.txt")
            bad_castep_reader.read_vibrational_or_phonon_data()

        with self.assertRaises(TypeError):
            # noinspection PyUnusedLocal
            CASTEPLoader(input_ab_initio_filename=1)

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


if __name__ == "__main__":
    unittest.main()
