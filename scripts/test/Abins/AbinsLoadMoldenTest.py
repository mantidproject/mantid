# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

import abins
from abins.input import MoldenLoader


class AbinsLoadMoldenTest(unittest.TestCase, abins.input.Tester):
    def test_molden(self):
        self.check(name="ethanol_cp2k_LoadMolden", loader=MoldenLoader)
