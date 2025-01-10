# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import abins.input
from abins.input import DMOL3Loader
import abins.test_helpers


class AbinsLoadDMOL3Test(unittest.TestCase, abins.input.Tester):
    """Check entire input files against expected outputs"""

    def tearDown(self):
        from mantid.kernel import ConfigService

        abins.test_helpers.remove_output_files(list_of_names=["_LoadDMOL3"], directory=ConfigService.getString("defaultsave.directory"))

    _gamma_dmol3 = "LTA_40_O2_LoadDMOL3"
    _gamma_no_h_dmol3 = "Na2SiF6_LoadDMOL3"
    _molecule_dmol3 = "methane_LoadDMOL3"
    _molecule_opt_dmol3 = "methane_opt_LoadDMOL3"

    def test_gamma_dmol3(self):
        self.check(name=self._gamma_dmol3, loader=DMOL3Loader)
        self.check(name=self._gamma_no_h_dmol3, loader=DMOL3Loader)

    def test_molecule_dmol3(self):
        self.check(name=self._molecule_dmol3, loader=DMOL3Loader)
        self.check(name=self._molecule_opt_dmol3, loader=DMOL3Loader)


if __name__ == "__main__":
    unittest.main()
