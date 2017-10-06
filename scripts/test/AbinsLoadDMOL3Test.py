from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import AbinsModules


class AbinsLoadDMOL3Test(unittest.TestCase, AbinsModules.GeneralLoadDFTTester):

    def tearDown(self):
        AbinsModules.AbinsTestHelpers.remove_output_files(list_of_names=["LoadDMOL3"])

        #  *************************** USE CASES ********************************************
    # ===================================================================================
    # | Use cases: Gamma point calculation for DMOL3                                    |
    # ===================================================================================
    _gamma_dmol3 = "LTA_40_O2_LoadDMOL3"
    _gamma_no_h_dmol3 = "Na2SiF6_LoadDMOL3"

    def test_gamma_dmol3(self):
        self.check(name=self._gamma_dmol3, loader=AbinsModules.LoadDMOL3)
        self.check(name=self._gamma_no_h_dmol3, loader=AbinsModules.LoadDMOL3)

if __name__ == '__main__':
    unittest.main()
