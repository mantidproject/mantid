# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.utils import AbsorptionCorrUtils


class AbsorptionCorrUtilsTest(unittest.TestCase):

    def test_donorws_nocharacterizations(self):
        self.assertRaises(RuntimeError, AbsorptionCorrUtils.create_absorption_input, '', None)

    def test_correction_methods(self):
        sample_ws, container_ws = AbsorptionCorrUtils.calculate_absorption_correction('', "None", None, "V", 1.0)

        self.assertIsNone(sample_ws)
        self.assertIsNone(container_ws)


if __name__ == '__main__':
    unittest.main()
