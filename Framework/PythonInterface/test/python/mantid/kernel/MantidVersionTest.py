# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import version_info
from testhelpers import assertRaisesNothing


class MaterialBuilderTest(unittest.TestCase):
    def test_version_info_major(self):
        assertRaisesNothing(self, int, version_info().major)

    def test_version_info_minor(self):
        assertRaisesNothing(self, int, version_info().major)

    def test_version_info_patch(self):
        self.assertTrue(version_info().major)


if __name__ == '__main__':
    unittest.main()
