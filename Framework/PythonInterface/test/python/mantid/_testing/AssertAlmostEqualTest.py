# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from mantid.testing import assert_almost_equal
from mantid.simpleapi import CreateWorkspace


class AssertAlmostEqualTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        ws1 = CreateWorkspace(DataX=[0, 1, 2, 3, 4, 5], DataY=[1, 1, 1, 1, 1, 1])
        ws2 = CreateWorkspace(DataX=[0.09, 1.09, 2.09, 3.09, 4.09, 5.09], DataY=[1, 1, 1, 1, 1, 1])
        ws3 = CreateWorkspace(DataX=[0, 2, 4, 6, 8, 10], DataY=[1, 1, 1, 1, 1, 1])
        self.ws1 = ws1
        self.ws2 = ws2
        self.ws3 = ws3

    def test_simple(self):
        assert_almost_equal(self.ws1, self.ws2, atol=1, rtol=1)

    def test_atol(self):
        # compare (ws1 - ws2) < atol
        assert_almost_equal(self.ws1, self.ws2, atol=0.1)

    def test_rtol(self):
        # compare (ws1 - ws2) / (0.5 * (ws1 + ws2)) < rtol
        assert_almost_equal(self.ws1, self.ws3, rtol=0.7)

    def test_raises(self):
        with self.assertRaises(ValueError):
            assert_almost_equal(self.ws1, self.ws2)


if __name__ == "__main__":
    unittest.main()
