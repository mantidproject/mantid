# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.utils.reflectometry import SpinStatesORSO


class PolarizationHelpersTest(unittest.TestCase):
    def test_ORSO_spin_state_definitions_are_imported_correctly(self):
        self.assertEqual("pp", SpinStatesORSO.PP)
        self.assertEqual("pm", SpinStatesORSO.PM)
        self.assertEqual("mp", SpinStatesORSO.MP)
        self.assertEqual("mm", SpinStatesORSO.MM)
        self.assertEqual("po", SpinStatesORSO.PO)
        self.assertEqual("mo", SpinStatesORSO.MO)
        self.assertEqual("spin_state_ORSO", SpinStatesORSO.LOG_NAME)


if __name__ == "__main__":
    unittest.main()
