# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import testhelpers
from mantid.kernel import NullValidator


class NullValidatorTest(unittest.TestCase):
    def test_NullValidator_can_be_default_constructed(self):
        testhelpers.assertRaisesNothing(self, NullValidator)


if __name__ == "__main__":
    unittest.main()
