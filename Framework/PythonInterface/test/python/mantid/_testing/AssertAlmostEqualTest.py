# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock
from mantid.testing import assert_almost_equal
from mantid.simpleapi import CreateWorkspace, CreateSingleValuedWorkspace


class AssertAlmostEqualTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        ws1 = CreateWorkspace(DataX=[0, 1, 2, 3, 4, 5], DataY=[1, 1, 1, 1, 1, 1])
        ws2 = CreateWorkspace(DataX=[0.09, 1.09, 2.09, 3.09, 4.09, 5.09], DataY=[1, 1, 1, 1, 1, 1])
        ws3 = CreateWorkspace(DataX=[0, 2, 4, 6, 8, 10], DataY=[1, 1, 1, 1, 1, 1])
        self.ws1 = ws1
        self.ws2 = ws2
        self.ws3 = ws3

    # SIMPLE CASES

    def test_noargs(self):
        another1 = self.ws1.clone()
        assert_almost_equal(self.ws1, another1)

    def test_simple(self):
        # compare (ws1 - ws2) < 1e-10
        assert_almost_equal(self.ws1, self.ws2, atol=1, rtol=1)

    def test_atol(self):
        # compare (ws1 - ws2) < atol
        assert_almost_equal(self.ws1, self.ws2, atol=0.1)

    def test_rtol(self):
        # compare (ws1 - ws2) / (0.5 * (ws1 + ws2)) < rtol
        assert_almost_equal(self.ws1, self.ws3, rtol=0.7)

    # VALIDATION TESTS

    def test_validate_forbidden_keys(self):
        with self.assertRaises(TypeError):
            assert_almost_equal(self.ws1, self.ws1, Workspace1=self.ws3)
        with self.assertRaises(TypeError):
            assert_almost_equal(self.ws1, self.ws1, Workspace2=self.ws3)
        with self.assertRaises(ValueError):
            assert_almost_equal(self.ws1, self.ws1, Tolerance=0.01)
        with self.assertRaises(ValueError):
            assert_almost_equal(self.ws1, self.ws1, ToleranceRelErr=True)

    def test_validate_atol(self):
        with self.assertRaises(ValueError):
            assert_almost_equal(self.ws1, self.ws2, atol=-1.0, rtol=1.0)

    def test_validate_rtol(self):
        with self.assertRaises(ValueError):
            assert_almost_equal(self.ws1, self.ws2, ato1=1.0, rtol=-1.0)

    # NEGATIVE TESTS

    def test_fail_unequal(self):
        with self.assertRaises(AssertionError):
            assert_almost_equal(self.ws1, self.ws2)

    def test_fail_absolute(self):
        with self.assertRaises(AssertionError):
            assert_almost_equal(self.ws1, self.ws2, atol=1e-3)

    def test_fail_relative(self):
        with self.assertRaises(AssertionError):
            assert_almost_equal(self.ws1, self.ws2, rtol=1.0e-4)

    # MIXED CASES

    def test_both_atol_fails(self):
        atol = 0.01
        rtol = 0.01
        ts1 = CreateSingleValuedWorkspace(1000000.1)
        ts2 = CreateSingleValuedWorkspace(1000000.0)
        # ensure rtol passes, atol fails separately
        assert_almost_equal(ts1, ts2, rtol=rtol)
        with self.assertRaises(AssertionError):
            assert_almost_equal(ts1, ts2, atol=atol)
        # ensure both fail
        with self.assertRaises(AssertionError):
            assert_almost_equal(ts1, ts2, atol=atol, rtol=rtol)

    def test_both_rtol_fails(self):
        atol = 0.01
        rtol = 0.01
        tes1 = CreateSingleValuedWorkspace(0.00001)
        tes2 = CreateSingleValuedWorkspace(0.00010)
        # ensure atol passes, rtol fails separately
        assert_almost_equal(tes1, tes2, atol=atol)
        with self.assertRaises(AssertionError):
            assert_almost_equal(tes1, tes2, rtol=rtol)
        # ensure both fail
        with self.assertRaises(AssertionError):
            assert_almost_equal(tes1, tes2, atol=atol, rtol=rtol)

    def test_both_both_fail(self):
        atol = 0.01
        rtol = 0.01
        ts1 = CreateSingleValuedWorkspace(100.01)
        ts2 = CreateSingleValuedWorkspace(200.02)
        # ensure rtol, atol fail separately
        with self.assertRaises(AssertionError):
            assert_almost_equal(ts1, ts2, atol=atol)
        with self.assertRaises(AssertionError):
            assert_almost_equal(ts1, ts2, rtol=rtol)
        # ensure both fail
        with self.assertRaises(AssertionError):
            assert_almost_equal(ts1, ts2, atol=atol, rtol=rtol)


if __name__ == "__main__":
    unittest.main()
