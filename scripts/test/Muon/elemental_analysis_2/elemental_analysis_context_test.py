# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext


class ElementalAnalysisContextTest(unittest.TestCase):
    def setUp(self):
        self.context = ElementalAnalysisContext()

    def test_name(self):
        self.assertEqual(self.context.name, "Elemental Analysis 2")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
