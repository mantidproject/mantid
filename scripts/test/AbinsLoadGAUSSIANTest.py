from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import AbinsModules


class AbinsLoadGAUSSIANTest(unittest.TestCase, AbinsModules.GeneralLoadDFTTester):

    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadGAUSSIAN"])

        #  *************************** USE CASES ********************************************
    # ===================================================================================
    # | Use cases: molecular calculation for GAUSSIAN                                    |
    # ===================================================================================
    _molecule_gaussian = "C6H5Cl-LoadGAUSSIAN"

    def test_molecule_gaussian(self):
        self._check(name=self._molecule_gaussian, loader=AbinsModules.LoadGAUSSIAN)

if __name__ == '__main__':
    unittest.main()
