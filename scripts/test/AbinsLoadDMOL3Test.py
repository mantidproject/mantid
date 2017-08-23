from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.simpleapi import logger
import AbinsModules


def old_python():
    """" Check if Python has proper version."""
    is_python_old = AbinsModules.AbinsTestHelpers.old_python()
    if is_python_old:
        logger.warning("Skipping AbinsLoadDMOL3Test because Python is too old.")
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
        self._check(name=self._gamma_dmol3, loader=AbinsModules.LoadDMOL3)
        self._check(name=self._gamma_no_h_dmol3, loader=AbinsModules.LoadDMOL3)

if __name__ == '__main__':
    unittest.main()
