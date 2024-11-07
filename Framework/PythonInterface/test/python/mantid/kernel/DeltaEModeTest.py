# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import DeltaEMode, DeltaEModeType


class DeltaEModeTest(unittest.TestCase):
    def test_availableTypes_contains_three_modes(self):
        modes = DeltaEMode.availableTypes()

        self.assertEqual(3, len(modes))
        self.assertTrue("Elastic" in modes)
        self.assertTrue("Direct" in modes)
        self.assertTrue("Indirect" in modes)

    def test_DeltaEModeType_has_three_attrs_corresponding_to_three_modes(self):
        self.assertTrue(hasattr(DeltaEModeType, "Elastic"))
        self.assertTrue(hasattr(DeltaEModeType, "Direct"))
        self.assertTrue(hasattr(DeltaEModeType, "Indirect"))


if __name__ == "__main__":
    unittest.main()
