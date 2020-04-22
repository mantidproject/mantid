# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import logger
from abins import GeneralLoadAbInitioTester, test_helpers, LoadGAUSSIAN


class AbinsLoadGAUSSIANTest(unittest.TestCase, GeneralLoadAbInitioTester):

    def tearDown(self):
        test_helpers.remove_output_files(list_of_names=["LoadGAUSSIAN"])

        #  *************************** USE CASES ********************************************
    # ===================================================================================
    # | Use cases: molecular calculation for GAUSSIAN03 Hartree Fock, Unix              |
    # ===================================================================================
    _gaussian_system1 = "C6H5Cl_LoadGAUSSIAN"

    def test_gaussian_1(self):
        self.check(name=self._gaussian_system1, loader=LoadGAUSSIAN)

    # ===================================================================================
    # | Use cases: molecular calculation for GAUSSIAN03 DFT, Win                        |
    # ===================================================================================
    _gaussian_system2 = "BENZENE4_g03_win_LoadGAUSSIAN"

    def test_gaussian_2(self):
        self.check(name=self._gaussian_system2, loader=LoadGAUSSIAN)

    # ===================================================================================
    # | Use cases: molecular calculation for GAUSSIAN09 DFT, Win                        |
    # ===================================================================================
    _gaussian_system3 = "BENZENE4_g09_win_LoadGAUSSIAN"

    def test_gaussian_3(self):
        self.check(name=self._gaussian_system3, loader=LoadGAUSSIAN)


if __name__ == '__main__':
    unittest.main()
