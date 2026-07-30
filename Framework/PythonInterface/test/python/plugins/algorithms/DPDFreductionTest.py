# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from DPDFreduction import DPDFreduction


class DPDFreductionTest(unittest.TestCase):
    def test_category_includes_deprecated_category(self):
        self.assertEqual("Inelastic\\Reduction;Deprecated", DPDFreduction().category())


if __name__ == "__main__":
    unittest.main()
