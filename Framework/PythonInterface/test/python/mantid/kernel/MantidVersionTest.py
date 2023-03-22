# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import version
from testhelpers import assertRaisesNothing


class MantidVersionTest(unittest.TestCase):
    def test_version_info_major(self):
        assertRaisesNothing(self, int, version().major)

    def test_version_info_minor(self):
        assertRaisesNothing(self, int, version().minor)

    def test_version_info_patch(self):
        # Patch can be one or more dot separated integers.
        self.assertRegex(version().patch, r"^\d+(\.\d+)*$")

    def test_version_info_tweak(self):
        assertRaisesNothing(self, print, version().tweak)

    def test_version_info_string(self):
        self.assertTrue("." in str(version))
        assertRaisesNothing(self, print, version())


if __name__ == "__main__":
    unittest.main()
